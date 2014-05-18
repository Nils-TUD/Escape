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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/idt.h>
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
#define PHYS2VIRT(x)			((void*)((uintptr_t)x + KERNEL_AREA))

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

static uintptr_t physModAddrs[MAX_MOD_COUNT];
static char mbbuf[PAGE_SIZE];
static size_t mbbufpos = 0;

extern void *_btext;
extern void *_ebss;
static BootInfo *mb;

static void *copyMBInfo(const void *info,size_t len) {
	void *res = mbbuf + mbbufpos;
	if(mbbufpos + len > sizeof(mbbuf))
		Util::panic("Multiboot-buffer too small");
	memcpy(mbbuf + mbbufpos,PHYS2VIRT(info),len);
	mbbufpos += len;
	return res;
}

void Boot::archStart(BootInfo *info) {
	BootModule *mod;
	/* copy mb-stuff into buffer */
	info = (BootInfo*)copyMBInfo(info,sizeof(BootInfo));
	info->cmdLine = (char*)copyMBInfo(info->cmdLine,strlen((char*)PHYS2VIRT(info->cmdLine)) + 1);
	info->modsAddr = (BootModule*)copyMBInfo(info->modsAddr,sizeof(BootModule) * info->modsCount);
	info->mmapAddr = (BootMemMap*)copyMBInfo(info->mmapAddr,info->mmapLength);
	info->drivesAddr = (BootDrive*)copyMBInfo(info->drivesAddr,info->drivesLength);
	mod = info->modsAddr;
	for(size_t i = 0; i < info->modsCount; i++) {
		mod->name = (char*)copyMBInfo(mod->name,strlen((char*)PHYS2VIRT(mod->name)) + 1);
		physModAddrs[i] = mod->modStart;
		mod++;
	}
	mb = info;
	if(mb->modsCount > MAX_MOD_COUNT)
		Util::panic("Too many modules (max %u)",MAX_MOD_COUNT);
	taskList.moduleCount = mb->modsCount;

	/* setup basic stuff */
	PageDir::init();
	GDT::init();
	/* necessary during initialization until we have a thread */
	Thread::setRunning(NULL);
	Serial::init();

	/* parse boot parameters (before PhysMem::init()) */
	int argc;
	const char **argv = Boot::parseArgs(mb->cmdLine,&argc);
	Config::parseBootParams(argc,argv);

	/* init physical memory and paging */
	Proc::preinit();
	PhysMem::init();

	/* clear screen here because of virtualbox-bug */
	Video::get().clearScreen();

	/* now map modules */
	mod = mb->modsAddr;
	for(size_t i = 0; i < mb->modsCount; i++) {
		size_t size = mod->modEnd - mod->modStart;
		mod->modStart = PageDir::makeAccessible(mod->modStart,BYTES_2_PAGES(size));
		mod->modEnd = mod->modStart + size;
		mod++;
	}

	Log::get().writef("Kernel parameters: %s\n",mb->cmdLine);
}

const BootInfo *Boot::getInfo() {
	return mb;
}

size_t Boot::getKernelSize() {
	uintptr_t start = (uintptr_t)&_btext;
	uintptr_t end = (uintptr_t)&_ebss;
	return end - start;
}

size_t Boot::getModuleSize() {
	if(mb->modsCount == 0)
		return 0;
	uintptr_t start = mb->modsAddr[0].modStart;
	uintptr_t end = mb->modsAddr[mb->modsCount - 1].modEnd;
	return end - start;
}

uintptr_t Boot::getModuleRange(const char *name,size_t *size) {
	BootModule *mod = mb->modsAddr;
	for(size_t i = 0; i < mb->modsCount; i++) {
		if(strcmp(mod->name,name) == 0) {
			*size = mod->modEnd - mod->modStart;
			return physModAddrs[i];
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
	BootModule *mod = mb->modsAddr;
	for(size_t i = 0; i < mb->modsCount; i++) {
		char *modname = (char*)Cache::alloc(12);
		itoa(modname,12,i);
		VFSNode *n = createObj<VFSFile>(KERNEL_PID,node,modname,(void*)mod->modStart,
				mod->modEnd - mod->modStart);
		if(!n || n->chmod(KERNEL_PID,S_IRUSR | S_IRGRP | S_IROTH) != 0)
			Util::panic("Unable to create/chmod mbmod-file for '%s'",modname);
		VFSNode::release(n);
		mod++;
	}
	VFSNode::release(node);

	/* load modules */
	loadedMods = true;
	mod = mb->modsAddr;
	for(size_t i = 0; i < mb->modsCount; i++) {
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
			res = Proc::exec(argv[0],argv,NULL,(void*)mod->modStart,mod->modEnd - mod->modStart);
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
	os.writef("flags=0x%x\n",mb->flags);
	if(CHECK_FLAG(mb->flags,0)) {
		os.writef("memLower=%d KB, memUpper=%d KB\n",mb->memLower,mb->memUpper);
	}
	if(CHECK_FLAG(mb->flags,1)) {
		os.writef("biosDriveNumber=%2X, part1=%2X, part2=%2X, part3=%2X\n",
			(uint)mb->bootDevice.drive,(uint)mb->bootDevice.partition1,
			(uint)mb->bootDevice.partition2,(uint)mb->bootDevice.partition3);
	}
	if(CHECK_FLAG(mb->flags,2)) {
		os.writef("cmdLine=%s\n",mb->cmdLine);
	}
	if(CHECK_FLAG(mb->flags,3)) {
		BootModule *mod = mb->modsAddr;
		os.writef("modsCount=%d:\n",mb->modsCount);
		for(size_t i = 0; i < mb->modsCount; i++) {
			os.writef("\t[%zu]: virt: %p..%p, phys: %p..%p\n",i,mod->modStart,mod->modEnd,
					physModAddrs[i],physModAddrs[i] + (mod->modEnd - mod->modStart));
			os.writef("\t     cmdline: %s\n",mod->name ? mod->name : "<NULL>");
			mod++;
		}
	}
	if(CHECK_FLAG(mb->flags,4)) {
		os.writef("a.out: tabSize=%d, strSize=%d, addr=0x%x\n",mb->syms.aDotOut.tabSize,
				mb->syms.aDotOut.strSize,mb->syms.aDotOut.addr);
	}
	else if(CHECK_FLAG(mb->flags,5)) {
		os.writef("ELF: num=%d, size=%d, addr=0x%x, shndx=0x%x\n",mb->syms.ELF.num,mb->syms.ELF.size,
			mb->syms.ELF.addr,mb->syms.ELF.shndx);
	}
	if(CHECK_FLAG(mb->flags,6)) {
		os.writef("mmapLength=%d, mmapAddr=%p\n",mb->mmapLength,mb->mmapAddr);
		os.writef("memory-map:\n");
		size_t x = 0;
		for(BootMemMap *mmap = (BootMemMap*)mb->mmapAddr;
			(uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (BootMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
			if(mmap != NULL) {
				os.writef("\t%d: addr=%#012Lx, size=%#012Lx, type=%s\n",
						x,mmap->baseAddr,mmap->length,
						mmap->type == MMAP_TYPE_AVAILABLE ? "free" : "used");
				x++;
			}
		}
	}
	if(CHECK_FLAG(mb->flags,7) && mb->drivesLength > 0) {
		BootDrive *drive = mb->drivesAddr;
		os.writef("Drives: (size=%u)\n",mb->drivesLength);
		for(size_t x = 0, i = 0; x < mb->drivesLength; x += drive->size) {
			os.writef("\t%d: no=%x, mode=%x, cyl=%u, heads=%u, sectors=%u\n",
					i,(uint)drive->number,(uint)drive->mode,(uint)drive->cylinders,
					(uint)drive->heads,(uint)drive->sectors);
		}
	}
}
