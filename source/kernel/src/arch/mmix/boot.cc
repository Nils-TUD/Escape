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
#include <sys/mem/pagedir.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/dynarray.h>
#include <sys/arch/mmix/mem/addrspace.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/timer.h>
#include <sys/task/smp.h>
#include <sys/task/terminator.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/log.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>

static const BootTask tasks[] = {
	{"Parsing bootinfo...",Boot::parseBootInfo},
	{"Parsing cmdline...",Boot::parseCmdline},
	{"Initializing physical memory-management...",PhysMem::init},
	{"Initializing address spaces...",AddressSpace::init},
	{"Initializing paging...",PageDir::init},
	{"Preinit processes...",Proc::preinit},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing processes...",Proc::init},
	{"Initializing scheduler...",Sched::init},
	{"Creating MB module files...",Boot::createModFiles},
	{"Start logging to VFS...",Log::vfsIsReady},
	{"Initializing timer...",Timer::init},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));
bool Boot::loadedMods = false;

static Boot::Module mods[MAX_PROG_COUNT];
static Boot::MemMap mmap;
static BootInfo *binfo;
static int bootState = 0;
static int bootFinished = 1;

void Boot::archStart(void *nfo) {
	binfo = (BootInfo*)nfo;
	CPU::setSpeed(binfo->cpuHz);
}

void Boot::parseBootInfo() {
	info.cmdLine = (char*)binfo->progs[0].command;
	info.modCount = binfo->progCount - 1;
	info.mods = mods;
	for(size_t i = 1; i < binfo->progCount; ++i) {
		info.mods[i - 1].name = (char*)binfo->progs[i].command;
		info.mods[i - 1].phys = binfo->progs[i].start & ~DIR_MAP_AREA;
		info.mods[i - 1].size = binfo->progs[i].size;
		info.mods[i - 1].virt = binfo->progs[i].start;
	}
	/* mark everything behind the modules as free */
	Module *last = info.mods + info.modCount - 1;
	mmap.baseAddr = ROUND_PAGE_UP(last->phys + last->size);
	mmap.size = binfo->memSize - mmap.baseAddr;
	mmap.type = 1;
	info.mmapCount = 1;
	info.mmap = &mmap;

	/* start idle-thread, load programs (without kernel) and wait for programs */
	bootFinished = info.modCount * 2 + 1;
}

int Boot::loadModules(A_UNUSED IntrptStackFrame *stack) {
	/* it's not good to do this twice.. */
	if(bootState == bootFinished)
		return 0;
	if(unittests != NULL)
		unittests();

	/* note that we can't do more than one Proc::clone at once here, because we have to leave the
	 * kernel immediatly to choose another stack (the created process gets our stack) */
	if(bootState == 0) {
		/* start idle-thread */
		Proc::startThread((uintptr_t)&thread_idle,T_IDLE,NULL);
		/* start termination-thread */
		Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
	}
	else if((bootState % 2) == 1) {
		size_t i = bootState / 2;
		/* clone proc */
		int child;
		if((child = Proc::clone(P_BOOT)) == 0) {
			int res,argc;
			/* parse args */
			const char **argv = Boot::parseArgs(mods[i].name,&argc);
			if(argc < 2)
				Util::panic("Invalid arguments for boot-module: %s\n",mods[i].name);
			/* exec */
			res = Proc::exec(argv[0],argv,NULL,(void*)mods[i].virt,mods[i].size);
			if(res < 0)
				Util::panic("Unable to exec boot-program %s: %d\n",mods[i].name,res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			Util::panic("Unable to clone process for boot-program %s: %d\n",mods[i].name,child);
	}
	else {
		size_t i = (bootState / 2) - 1;
		VFSNode *node = NULL;
		int argc;
		const char **argv = Boot::parseArgs(mods[i].name,&argc);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		while(VFSNode::request(argv[1],NULL,&node,NULL,VFS_NOACCESS,0) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			Timer::sleepFor(Thread::getRunning()->getTid(),20,true);
			Thread::switchAway();
		}
		VFSNode::release(node);
	}
	bootState++;

	/* if we're done, mount root filesystem */
	if(bootState == bootFinished) {
		Proc *p = Proc::getByPid(Proc::getRunning());
		OpenFile *file;
		const char *rootDev = Config::getStr(Config::ROOT_DEVICE);
		int res;
		if((res = VFS::openPath(p->getPid(),VFS_READ | VFS_WRITE | VFS_MSGS,0,rootDev,&file)) < 0)
			Util::panic("Unable to open root device '%s': %s",rootDev,strerror(res));
		if((res = MountSpace::mount(p,"/",file)) < 0)
			Util::panic("Unable to mount /: %s",strerror(res));
	}

	/* TODO */
#if 0
	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
#endif
	return bootState == bootFinished ? 0 : 1;
}
