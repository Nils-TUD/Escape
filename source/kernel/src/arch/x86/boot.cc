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
#include <sys/arch/x86/fpu.h>
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

static void mapModules();

static const BootTask tasks[] = {
	{"Parsing multiboot info...",Boot::parseBootInfo},
	{"Initializing Serial...",Serial::init},
	{"Initializing PageDir...",PageDir::init},
	{"Initializing GDT...",GDT::init},
	{"Parsing cmdline...",Boot::parseCmdline},
	{"Pre-initializing processes...",Proc::preinit},
	{"Initializing physical memory management...",PhysMem::init},
	{"Map modules...",mapModules},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing LAPIC...",LAPIC::init},
	{"Initializing ACPI...",ACPI::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing GDT for BSP...",GDT::initBSP},
	{"Initializing CPU...",CPU::detect},
	{"Initializing FPU...",FPU::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing processes...",Proc::init},
	{"Creating ACPI files...",ACPI::createFiles},
	{"Creating MB module files...",Boot::createModFiles},
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

static MultiBootInfo *mbinfo;

static void *copyMBInfo(uintptr_t info,size_t len) {
	void *res = mbbuf + mbbufpos;
	if(mbbufpos + len > sizeof(mbbuf))
		Util::panic("Multiboot-buffer too small");
	memcpy(mbbuf + mbbufpos,(const void*)info,len);
	mbbufpos += len;
	return res;
}

void Boot::archStart(void *nfo) {
	mbinfo = (MultiBootInfo*)nfo;
	taskList.moduleCount = mbinfo->modsCount;
}

void Boot::parseBootInfo() {
	BootModule *mbmod;
	/* copy mb-stuff into our own datastructure */
	info.cmdLine = (char*)copyMBInfo(mbinfo->cmdLine,strlen((char*)(uintptr_t)mbinfo->cmdLine) + 1);
	info.modCount = mbinfo->modsCount;

	/* copy memory map */
	info.mmap = (MemMap*)(mbbuf + mbbufpos);
	info.mmapCount = 0;
	for(BootMemMap *mmap = (BootMemMap*)(uintptr_t)mbinfo->mmapAddr;
			(uintptr_t)mmap < (uintptr_t)mbinfo->mmapAddr + mbinfo->mmapLength;
			mmap = (BootMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		info.mmap[info.mmapCount].baseAddr = mmap->baseAddr;
		info.mmap[info.mmapCount].size = mmap->length;
		info.mmap[info.mmapCount].type = mmap->type;
		mbbufpos += sizeof(MemMap);
		info.mmapCount++;
	}

	/* copy modules (do that here because we can't access the multiboot-info afterwards anymore) */
	info.mods = (Module*)(mbbuf + mbbufpos);
	mbbufpos += sizeof(Module) * mbinfo->modsCount;
	mbmod = (BootModule*)(uintptr_t)mbinfo->modsAddr;
	for(size_t i = 0; i < mbinfo->modsCount; i++) {
		info.mods[i].phys = mbmod->modStart;
		info.mods[i].virt = 0;
		info.mods[i].size = mbmod->modEnd - mbmod->modStart;
		info.mods[i].name = (char*)copyMBInfo(mbmod->name,strlen((char*)(uintptr_t)mbmod->name) + 1);
		mbmod++;
	}
}

static void mapModules() {
	/* make modules accessible */
	for(auto mod = Boot::modsBegin(); mod != Boot::modsEnd(); ++mod)
		mod->virt = PageDir::makeAccessible(mod->phys,BYTES_2_PAGES(mod->size));
}

int Boot::loadModules(A_UNUSED IntrptStackFrame *stack) {
	char loadingStatus[256];

	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;
	if(unittests != NULL)
		unittests();

	/* load modules */
	loadedMods = true;
	for(auto mod = Boot::modsBegin(); mod != Boot::modsEnd(); ++mod) {
		/* parse args */
		int argc;
		const char **argv = Boot::parseArgs(mod->name,&argc);
		/* ignore modules with just a path; this might be a ramdisk */
		if(argc == 1) {
			Boot::taskFinished();
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
			int res = Proc::exec(argv[0],argv,NULL,(void*)mod->virt,mod->size);
			if(res < 0)
				Util::panic("Unable to exec boot-program %s: %d\n",argv[0],res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			Util::panic("Unable to clone process for boot-program %s: %d\n",argv[0],child);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		VFSNode *node = NULL;
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
	}

	/* now all boot-modules are loaded, so mount root filesystem */
	Proc *p = Proc::getByPid(Proc::getRunning());
	int res;
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
