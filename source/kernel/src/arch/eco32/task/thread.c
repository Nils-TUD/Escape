/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/cpu.h>
#include <esc/sllist.h>
#include <assert.h>
#include <errors.h>

int thread_initArch(sThread *t) {
	UNUSED(t);
	/* nothing to do */
	return 0;
}

int thread_cloneArch(const sThread *src,sThread *dst,bool cloneProc) {
	if(!cloneProc) {
		if(pmem_getFreeFrames(MM_DEF) < INITIAL_STACK_PAGES)
			return ERR_NOT_ENOUGH_MEM;

		/* add a new stack-region */
		dst->stackRegions[0] = vmm_add(dst->proc,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
		if(dst->stackRegions[0] < 0)
			return dst->stackRegions[0];
	}
	return 0;
}

void thread_freeArch(sThread *t) {
	if(t->stackRegions[0] >= 0) {
		vmm_remove(t->proc,t->stackRegions[0]);
		t->stackRegions[0] = -1;
	}
}

void thread_switchTo(tTid tid) {
	sThread *cur = thread_getRunning();
	/* finish kernel-time here since we're switching the process */
	if(tid != cur->tid) {
		uint64_t kcstart = cur->stats.kcycleStart;
		if(kcstart > 0) {
			uint64_t cycles = cpu_rdtsc();
			cur->stats.kcycleCount.val64 += cycles - kcstart;
		}

		if(!thread_save(&cur->save)) {
			sThread *old;
			sThread *t = thread_getById(tid);
			vassert(t != NULL,"Thread with tid %d not found",tid);

			/* mark old process ready, if it should not be blocked, killed or something */
			if(cur->state == ST_RUNNING)
				sched_setReady(cur);

			old = cur;
			thread_setRunning(t);
			cur = t;

			/* set used */
			cur->stats.schedCount++;
			vmm_setTimestamp(cur,timer_getTimestamp());
			sched_setRunning(cur);

			thread_resume(cur->proc->pagedir,&cur->save,cur->kstackFrame);
		}

		/* now start kernel-time again */
		cur = thread_getRunning();
		cur->stats.kcycleStart = cpu_rdtsc();
	}

	thread_killDead();
}


#if DEBUGGING

void thread_dbg_printState(const sThreadRegs *state) {
	vid_printf("\tState:\n",state);
	vid_printf("\t\t$16 = %#08x\n",state->r16);
	vid_printf("\t\t$17 = %#08x\n",state->r17);
	vid_printf("\t\t$18 = %#08x\n",state->r18);
	vid_printf("\t\t$19 = %#08x\n",state->r19);
	vid_printf("\t\t$20 = %#08x\n",state->r20);
	vid_printf("\t\t$21 = %#08x\n",state->r21);
	vid_printf("\t\t$22 = %#08x\n",state->r22);
	vid_printf("\t\t$23 = %#08x\n",state->r23);
	vid_printf("\t\t$29 = %#08x\n",state->r29);
	vid_printf("\t\t$30 = %#08x\n",state->r30);
	vid_printf("\t\t$31 = %#08x\n",state->r31);
}

#endif
