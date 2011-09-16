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
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/cow.h>
#include <sys/mem/sharedmem.h>
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
#include <sys/log.h>
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/video.h>
#include <sys/util.h>
#include <string.h>

static const sBootTask tasks[] = {
	{"Initializing physical memory-management...",pmem_init},
	{"Initializing paging...",paging_init},
	{"Preinit processes...",proc_preinit},
	{"Initializing dynarray...",dyna_init},
	{"Initializing SMP...",smp_init},
	{"Initializing VFS...",vfs_init},
	{"Initializing event system...",ev_init},
	{"Initializing processes...",proc_init},
	{"Initializing scheduler...",sched_init},
	{"Initializing terminator...",term_init},
#ifndef TESTING
	{"Start logging to VFS...",log_vfsIsReady},
#endif
	{"Initializing virtual memory-management...",vmm_init},
	{"Initializing copy-on-write...",cow_init},
	{"Initializing shared memory...",shm_init},
	{"Initializing timer...",timer_init},
	{"Initializing signal handling...",sig_init},
};
const sBootTaskList bootTaskList = {
	.tasks = tasks,
	.count = ARRAY_SIZE(tasks)
};

static bool loadedMods = false;
static sLoadProg progs[MAX_PROG_COUNT];
static sBootInfo info;

void boot_arch_start(sBootInfo *binfo) {
	int argc;
	const char **argv;
	/* make a copy of the bootinfo, since the location it is currently stored in will be overwritten
	 * shortly */
	memcpy(&info,binfo,sizeof(sBootInfo));
	info.progs = progs;
	memcpy((void*)info.progs,binfo->progs,sizeof(sLoadProg) * binfo->progCount);

	vid_init();

	/* parse the boot parameter */
	argv = boot_parseArgs(binfo->progs[0].command,&argc);
	conf_parseBootParams(argc,argv);
}

const sBootInfo *boot_getInfo(void) {
	return &info;
}

size_t boot_getKernelSize(void) {
	return progs[0].size;
}

size_t boot_getModuleSize(void) {
	uintptr_t start = progs[1].start;
	uintptr_t end = progs[info.progCount - 1].start + progs[info.progCount - 1].size;
	return end - start;
}

size_t boot_getUsableMemCount(void) {
	return info.memSize;
}

int boot_loadModules(A_UNUSED sIntrptStackFrame *stack) {
	size_t i;
	inode_t nodeNo;
	int child;

	/* it's not good to do this twice.. */
	if(loadedMods)
		return 0;

	/* start idle-thread */
	proc_startThread((uintptr_t)&thread_idle,T_IDLE,NULL);

	loadedMods = true;
	for(i = 1; i < info.progCount; i++) {
		/* parse args */
		int argc;
		const char **argv = boot_parseArgs(progs[i].command,&argc);
		if(argc < 2)
			util_panic("Invalid arguments for boot-module: %s\n",progs[i].command);

		/* clone proc */
		if((child = proc_clone(0)) == 0) {
			int res = proc_exec(progs[i].command,argv,(void*)progs[i].start,progs[i].size);
			if(res < 0)
				util_panic("Unable to exec boot-program %s: %d\n",progs[i].command,res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			util_panic("Unable to clone process for boot-program %s: %d\n",progs[i].command,child);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		while(vfs_node_resolvePath(argv[1],&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			timer_sleepFor(thread_getRunning()->tid,20,true);
			thread_switch();
		}
	}

	/* TODO */
#if 0
	/* start the swapper-thread. it will never return */
	proc_startThread((uintptr_t)&swap_start,0,NULL);
#endif
	proc_startThread((uintptr_t)&term_start,0,NULL);

	/* if not requested otherwise, from now on, print only to log */
	if(!conf_get(CONF_LOG2SCR))
		vid_setTargets(TARGET_LOG);
	return 0;
}

void boot_print(void) {
	size_t i;
	vid_printf("Memory size: %zu bytes\n",info.memSize);
	vid_printf("Disk size: %zu bytes\n",info.diskSize);
	vid_printf("Boot modules:\n");
	/* skip kernel */
	for(i = 1; i < info.progCount; i++) {
		vid_printf("\t%s [%p .. %p]\n",info.progs[i].command,
				info.progs[i].start,info.progs[i].start + info.progs[i].size);
	}
}
