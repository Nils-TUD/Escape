/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/copyonwrite.h>
#include <sys/mem/dynarray.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
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
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <string.h>

static const BootTask tasks[] = {
	{"Initializing physical memory-management...",PhysMem::init},
	{"Initializing paging...",PageDir::init},
	{"Preinit processes...",Proc::preinit},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing VFS...",VFS::init},
	{"Initializing event system...",Event::init},
	{"Initializing processes...",Proc::init},
	{"Initializing scheduler...",Sched::init},
	{"Start logging to VFS...",Log::vfsIsReady},
	{"Initializing timer...",Timer::init},
	{"Initializing signal handling...",Signals::init},
};
BootTaskList Boot::taskList(tasks,ARRAY_SIZE(tasks));
bool Boot::loadedMods = false;

static LoadProg progs[MAX_PROG_COUNT];
static BootInfo info;

void Boot::archStart(BootInfo *binfo) {
	/* clear screen here because of virtualbox-bug */
	Video::get().clearScreen();
	/* make a copy of the bootinfo, since the location it is currently stored in will be overwritten
	 * shortly */
	memcpy(&info,binfo,sizeof(BootInfo));
	info.progs = progs;
	memcpy((void*)info.progs,binfo->progs,sizeof(LoadProg) * binfo->progCount);

	/* parse the boot parameter */
	int argc;
	const char **argv = parseArgs(binfo->progs[0].command,&argc);
	Config::parseBootParams(argc,argv);
}

const BootInfo *Boot::getInfo() {
	return &info;
}

size_t Boot::getKernelSize() {
	return progs[0].size;
}

size_t Boot::getModuleSize() {
	uintptr_t start = progs[1].start;
	uintptr_t end = progs[info.progCount - 1].start + progs[info.progCount - 1].size;
	return end - start;
}

uintptr_t Boot::getModuleRange(const char *name,size_t *size) {
	for(size_t i = 1; i < info.progCount; i++) {
		if(strcmp(progs[i].command,name) == 0) {
			*size = progs[i].size;
			return progs[i].start;
		}
	}
	return 0;
}

int Boot::loadModules(A_UNUSED IntrptStackFrame *stack) {
	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;

	/* start idle-thread */
	Proc::startThread((uintptr_t)&thread_idle,T_IDLE,NULL);

	loadedMods = true;
	for(size_t i = 1; i < info.progCount; i++) {
		/* parse args */
		int argc;
		const char **argv = parseArgs(progs[i].command,&argc);
		if(argc < 2)
			Util::panic("Invalid arguments for boot-module: %s\n",progs[i].command);

		/* clone proc */
		int child;
		if((child = Proc::clone(P_BOOT)) == 0) {
			int res = Proc::exec(argv[0],argv,(void*)progs[i].start,progs[i].size);
			if(res < 0)
				Util::panic("Unable to exec boot-program %s: %d\n",progs[i].command,res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			Util::panic("Unable to clone process for boot-program %s: %d\n",progs[i].command,child);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		VFSNode *node;
		while(VFSNode::request(argv[1],&node,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			Timer::sleepFor(Thread::getRunning()->getTid(),20,true);
			Thread::switchAway();
		}
		VFSNode::release(node);
	}

	/* TODO */
#if 0
	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
#endif
	Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
	return 0;
}

void Boot::print(OStream &os) {
	os.writef("Memory size: %zu bytes\n",info.memSize);
	os.writef("Disk size: %zu bytes\n",info.diskSize);
	os.writef("Boot modules:\n");
	/* skip kernel */
	for(size_t i = 1; i < info.progCount; i++) {
		os.writef("\t%s [%p .. %p]\n",info.progs[i].command,
				info.progs[i].start,info.progs[i].start + info.progs[i].size);
	}
}
