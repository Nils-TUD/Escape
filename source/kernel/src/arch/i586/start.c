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
#include <sys/arch/i586/task/thread.h>
#include <sys/arch/i586/mem/paging.h>
#include <sys/arch/i586/gdt.h>
#include <sys/task/thread.h>
#include <sys/task/elf.h>
#include <sys/task/smp.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <assert.h>

/* make gcc happy */
void bspstart(sBootInfo *mbp);
uintptr_t smpstart(void);
void apstart(void);

static uint8_t initloader[] = {
#if DEBUGGING
#	include "../../../../build/i586-debug/user_initloader.dump"
#else
#	include "../../../../build/i586-release/user_initloader.dump"
#endif
};

void bspstart(sBootInfo *mbp) {
	/* init the kernel */
	boot_init(mbp,true);
}

uintptr_t smpstart(void) {
	sThread *t;
	sStartupInfo info;

	/* start an idle-thread for each cpu */
	sSLNode *n;
	const sSLList *cpus = smp_getCPUs();
	for(n = sll_begin(cpus); n != NULL; n = n->next) {
		proc_startThread((uintptr_t)&thread_idle,T_IDLE,NULL);
#if 0
		if(proc_startThread(thread_idle,T_IDLE,NULL) == thread_getRunning()->tid) {
			if(!smp_isBSP()) {
				/* unlock the trampoline */
				uintptr_t tramp = TRAMPOLINE_ADDR;
				*(uint32_t*)((tramp | KERNEL_START) + 6) = 0;
			}
			/* now start idling */
			thread_idle();
			util_panic("Idle returned");
		}
#endif
	}

	/* start all APs */
	smp_start();

	/* load initloader */
	if(elf_loadFromMem(initloader,sizeof(initloader),&info) < 0)
		util_panic("Unable to load initloader");
	/* give the process some stack pages */
	t = thread_getRunning();
	thread_addInitialStack(t);
	return info.progEntry;
}

void apstart(void) {
	while(1);
	sProc *p = proc_getByPid(0);
	paging_activate(p->pagedir.own);
	gdt_init_ap();

	boot_print();
	thread_initialSwitch();
}
