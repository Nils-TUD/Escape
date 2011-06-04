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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <sys/cpu.h>
#include <assert.h>

int thread_initArch(sThread *t) {
	t->archAttr.fpuState = NULL;
	return 0;
}

int thread_cloneArch(const sThread *src,sThread *dst,bool cloneProc) {
	UNUSED(cloneProc);
	fpu_cloneState(&(dst->archAttr.fpuState),src->archAttr.fpuState);
	return 0;
}

void thread_freeArch(sThread *t) {
	fpu_freeState(&t->archAttr.fpuState);
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
			uintptr_t tlsStart,tlsEnd;
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

			if(old->proc != cur->proc) {
				/* remove the io-map. it will be set as soon as the process accesses an io-port
				 * (we'll get an exception) */
				tss_removeIOMap();
				tss_setStackPtr(cur->proc->flags & P_VM86);
				paging_setCur(cur->proc->pagedir);
			}

			/* set TLS-segment in GDT */
			if(cur->tlsRegion >= 0) {
				vmm_getRegRange(cur->proc,cur->tlsRegion,&tlsStart,&tlsEnd);
				gdt_setTLS(tlsStart,tlsEnd - tlsStart);
			}
			else
				gdt_setTLS(0,0xFFFFFFFF);
			/* lock the FPU so that we can save the FPU-state for the previous process as soon
			 * as this one wants to use the FPU */
			fpu_lockFPU();
			thread_resume(cur->proc->pagedir,&cur->save,
					sll_length(cur->proc->threads) > 1 ? cur->kstackFrame : 0);
		}

		/* now start kernel-time again */
		cur = thread_getRunning();
		cur->stats.kcycleStart = cpu_rdtsc();
	}

	thread_killDead();
}


#if DEBUGGING

void thread_dbg_printState(const sThreadRegs *state) {
	vid_printf("\tState @ 0x%08Px:\n",state);
	vid_printf("\t\tesp = %#08x\n",state->esp);
	vid_printf("\t\tedi = %#08x\n",state->edi);
	vid_printf("\t\tesi = %#08x\n",state->esi);
	vid_printf("\t\tebp = %#08x\n",state->ebp);
	vid_printf("\t\teflags = %#08x\n",state->eflags);
}

#endif
