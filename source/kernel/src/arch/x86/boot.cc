/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/arch/x86/idt.h>
#include <sys/arch/x86/gdt.h>
#include <sys/arch/x86/serial.h>
#include <sys/arch/x86/pic.h>
#include <sys/arch/x86/acpi.h>
#include <sys/arch/x86/ioapic.h>
#include <sys/task/timer.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/dynarray.h>
#include <sys/mem/physmemareas.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/smp.h>
#include <sys/task/terminator.h>
#include <sys/vfs/file.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/log.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/cppsupport.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#define MAX_MOD_COUNT			10
#define CHECK_FLAG(flags,bit)	(flags & (1 << bit))

static const BootTask tasks[] = {
	{"Initializing dynarray...",DynArray::init},
	{"Initializing LAPIC...",LAPIC::init},
	{"Initializing ACPI...",ACPI::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing GDT...",GDT::initBSP},
	{"Initializing CPU...",CPU::detect},
	{"Initializing FPU...",FPU::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing processes...",Proc::init},
	{"Creating ACPI files...",ACPI::create_files},
	{"Initializing scheduler...",Sched::init},
	{"Start logging to VFS...",Log::vfsIsReady},
	{"Initializing interrupts...",Interrupts::init},
	{"Initializing IDT...",IDT::init},
	{"Initializing timer...",Timer::init},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));
bool Boot::loadedMods = false;

static char mbbuf[PAGE_SIZE];
static size_t mbbufpos = 0;

extern void *_btext;
extern void *_ebss;
static BootInfo mb;

static void *copyMBInfo(uintptr_t info,size_t len) {
	void *res = mbbuf + mbbufpos;
	if(mbbufpos + len > sizeof(mbbuf))
		Util::panic("Multiboot-buffer too small");
	memcpy(mbbuf + mbbufpos,(const void*)info,len);
	mbbufpos += len;
	return res;
}

void Boot::archStart(void *nfo) {
	{
		MultiBootInfo *info = (MultiBootInfo*)nfo;
		BootModule *mbmod;
		/* copy mb-stuff into our own datastructure */
		mb.cmdLine = (char*)copyMBInfo(info->cmdLine,strlen((char*)(uintptr_t)info->cmdLine) + 1);
		mb.modCount = info->modsCount;
		if(mb.modCount > MAX_MOD_COUNT)
			Util::panic("Too many modules (max %u)",MAX_MOD_COUNT);
		taskList.moduleCount = mb.modCount;

		/* copy memory map */
		mb.mmap = (BootMemMap*)(mbbuf + mbbufpos);
		mb.mmapCount = 0;
		for(BootMemMap *mmap = (BootMemMap*)(uintptr_t)info->mmapAddr;
				(uintptr_t)mmap < (uintptr_t)info->mmapAddr + info->mmapLength;
				mmap = (BootMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
			mb.mmap[mb.mmapCount].baseAddr = mmap->baseAddr;
			mb.mmap[mb.mmapCount].length = mmap->length;
			mb.mmap[mb.mmapCount].size = mmap->size;
			mb.mmap[mb.mmapCount].type = mmap->type;
			mbbufpos += sizeof(BootMemMap);
			mb.mmapCount++;
		}

		/* copy modules (do that here because we can't access the multiboot-info afterwards anymore) */
		mb.mods = (BootInfo::Module*)(mbbuf + mbbufpos);
		mbbufpos += sizeof(BootInfo::Module) * info->modsCount;
		mbmod = (BootModule*)(uintptr_t)info->modsAddr;
		for(size_t i = 0; i < info->modsCount; i++) {
			mb.mods[i].phys = mbmod->modStart;
			mb.mods[i].virt = 0;
			mb.mods[i].size = mbmod->modEnd - mbmod->modStart;
			mb.mods[i].name = (char*)copyMBInfo(mbmod->name,strlen((char*)(uintptr_t)mbmod->name) + 1);
			mbmod++;
		}
	}

	/* setup basic stuff */
	Serial::init();
	PageDir::init();
	GDT::init();
	/* necessary during initialization until we have a thread */
	Thread::setRunning(NULL);

	/* parse boot parameters (before PhysMem::init()) */
	int argc;
	const char **argv = Boot::parseArgs((char*)mb.cmdLine,&argc);
	Config::parseBootParams(argc,argv);

	/* init physical memory and paging */
	Proc::preinit();
	PhysMem::init();

	/* clear screen here because of virtualbox-bug */
	Video::get().clearScreen();

	/* now map the modules */
	for(size_t i = 0; i < mb.modCount; i++)
		mb.mods[i].virt = PageDir::makeAccessible(mb.mods[i].phys,BYTES_2_PAGES(mb.mods[i].size));

	Log::get().writef("Kernel parameters: %s\n",mb.cmdLine);
}

const BootInfo *Boot::getInfo() {
	return &mb;
}

size_t Boot::getKernelSize() {
	uintptr_t start = (uintptr_t)&_btext;
	uintptr_t end = (uintptr_t)&_ebss;
	return end - start;
}

size_t Boot::getModuleSize() {
	if(mb.modCount == 0)
		return 0;
	uintptr_t start = mb.mods[0].virt;
	uintptr_t end = mb.mods[mb.modCount - 1].virt + mb.mods[mb.modCount - 1].size;
	return end - start;
}

uintptr_t Boot::getModuleRange(const char *name,size_t *size) {
	BootInfo::Module *mod = mb.mods;
	for(size_t i = 0; i < mb.modCount; i++) {
		if(strcmp(mod->name,name) == 0) {
			*size = mod->size;
			return mod->phys;
		}
		mod++;
	}
	return 0;
}

int Boot::loadModules(A_UNUSED IntrptStackFrame *stack) {
	char loadingStatus[256];

	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;
	if(unittests != NULL)
		unittests();

	/* create module files */
	VFSNode *node = NULL;
	int res = VFSNode::request("/system/mbmods",NULL,&node,NULL,VFS_WRITE,0);
	if(res < 0)
		Util::panic("Unable to resolve /system/mbmods");
	BootInfo::Module *mod = mb.mods;
	for(size_t i = 0; i < mb.modCount; i++) {
		char *modname = (char*)Cache::alloc(12);
		itoa(modname,12,i);
		VFSNode *n = createObj<VFSFile>(KERNEL_PID,node,modname,(void*)mod->virt,mod->size);
		if(!n || n->chmod(KERNEL_PID,S_IRUSR | S_IRGRP | S_IROTH) != 0)
			Util::panic("Unable to create/chmod mbmod-file for '%s'",modname);
		VFSNode::release(n);
		mod++;
	}
	VFSNode::release(node);

	/* load modules */
	loadedMods = true;
	mod = mb.mods;
	for(size_t i = 0; i < mb.modCount; i++) {
		/* parse args */
		int argc;
		const char **argv = Boot::parseArgs(mod->name,&argc);
		/* ignore modules with just a path; this might be a ramdisk */
		if(argc == 1) {
			Boot::taskFinished();
			mod++;
			continue;
		}
		if(argc < 2)
			Util::panic("Invalid arguments for multiboot-module: %s\n",mod->name);

		strcpy(loadingStatus,"Loading ");
		strcat(loadingStatus,argv[0]);
		strcat(loadingStatus,"...");
		Boot::taskStarted(loadingStatus);

		int child;
		if((child = Proc::clone(P_BOOT)) == 0) {
			res = Proc::exec(argv[0],argv,NULL,(void*)mod->virt,mod->size);
			if(res < 0)
				Util::panic("Unable to exec boot-program %s: %d\n",argv[0],res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			Util::panic("Unable to clone process for boot-program %s: %d\n",argv[0],child);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		node = NULL;
		while(VFSNode::request(argv[1],NULL,&node,NULL,VFS_NOACCESS,0) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			Timer::sleepFor(Thread::getRunning()->getTid(),20,true);
			Thread::switchAway();
		}
		VFSNode::release(node);

		Boot::taskFinished();
		mod++;
	}

	/* now all boot-modules are loaded, so mount root filesystem */
	Proc *p = Proc::getByPid(Proc::getRunning());
	OpenFile *file;
	const char *rootDev = Config::getStr(Config::ROOT_DEVICE);
	if((res = VFS::openPath(p->getPid(),VFS_READ | VFS_WRITE | VFS_MSGS,0,rootDev,&file)) < 0)
		Util::panic("Unable to open root device '%s': %s",rootDev,strerror(res));
	if((res = MountSpace::mount(p,"/",file)) < 0)
		Util::panic("Unable to mount /: %s",strerror(res));

	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
	/* start the terminator */
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
	return 0;
}

void Boot::print(OStream &os) {
	os.writef("cmdLine: %s\n",mb.cmdLine);

	os.writef("modCount: %zu\n",mb.modCount);
	for(size_t i = 0; i < mb.modCount; i++) {
		os.writef("\t[%zu]: virt: %p..%p, phys: %p..%p\n",i,
			mb.mods[i].virt,mb.mods[i].virt + mb.mods[i].size,
			mb.mods[i].phys,mb.mods[i].phys + mb.mods[i].size);
		os.writef("\t     cmdline: %s\n",mb.mods[i].name ? mb.mods[i].name : "<NULL>");
	}

	os.writef("mmapCount: %zu\n",mb.mmapCount);
	for(size_t i = 0; i < mb.mmapCount; i++) {
		os.writef("\t%zu: addr=%#012Lx, size=%#012Lx, type=%s\n",
				i,mb.mmap[i].baseAddr,mb.mmap[i].length,
				mb.mmap[i].type == MMAP_TYPE_AVAILABLE ? "free" : "used");
	}
}
