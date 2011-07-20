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
#include <sys/task/thread.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <sys/config.h>
#include <sys/cpu.h>
#include <assert.h>
#include <errors.h>

extern bool thread_save(sThreadRegs *saveArea);
extern bool thread_resume(uintptr_t pageDir,const sThreadRegs *saveArea,frameno_t kstackFrame);
static void thread_doSwitch(sThread *cur,sThread *old);

int thread_initArch(sThread *t) {
	/* setup kernel-stack for us */
	frameno_t stackFrame = pmem_allocate();
	paging_map(KERNEL_STACK,&stackFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	t->kstackFrame = stackFrame;
	t->archAttr.fpuState = NULL;
	return 0;
}

void thread_addInitialStack(sThread *t) {
	assert(t->tid == INIT_TID);
	t->stackRegions[0] = vmm_add(t->proc,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[0] >= 0);
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
	fpu_cloneState(&(dst->archAttr.fpuState),src->archAttr.fpuState);
	return 0;
}

void thread_freeArch(sThread *t) {
	if(t->stackRegions[0] >= 0) {
		vmm_remove(t->proc,t->stackRegions[0]);
		t->stackRegions[0] = -1;
	}
	/* if there is just one thread left we have to map his kernel-stack again because we won't
	 * do it for single-thread-processes on a switch for performance-reasons */
	if(sll_length(t->proc->threads) == 1) {
		frameno_t stackFrame = ((sThread*)sll_get(t->proc->threads,0))->kstackFrame;
		paging_mapTo(&t->proc->pagedir,KERNEL_STACK,&stackFrame,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	}
	fpu_freeState(&t->archAttr.fpuState);
}

int thread_finishClone(sThread *t,sThread *nt) {
	UNUSED(t);
	ulong *src;
	size_t i;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	ulong *dst = (ulong*)paging_mapToTemp(&nt->kstackFrame,1);

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (ulong*)KERNEL_STACK;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	paging_unmapFromTemp(1);
	/* parent */
	return 0;
}

void thread_initialSwitch(void) {
	sThread *cur = sched_perform();
	thread_setRunning(cur);
	thread_doSwitch(cur,NULL);
}

void thread_switchTo(tid_t tid) {
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
			if(cur->flags & T_IDLE)
				thread_pushIdle(cur);

			old = cur;
			thread_setRunning(t);
			cur = t;

			thread_doSwitch(cur,old);
		}

		/* now start kernel-time again */
		cur = thread_getRunning();
		cur->stats.kcycleStart = cpu_rdtsc();
	}

	thread_killDead();
}

static void thread_doSwitch(sThread *cur,sThread *old) {
	uintptr_t tlsStart,tlsEnd;
	/* set used */
	cur->stats.schedCount++;
	if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
		vmm_setTimestamp(cur,timer_getTimestamp());
	sched_setRunning(cur);

	if(!old || old->proc != cur->proc) {
		/* remove the io-map. it will be set as soon as the process accesses an io-port
		 * (we'll get an exception) */
		tss_removeIOMap();
		tss_setStackPtr(cur->proc->flags & P_VM86);
	}

	/* set TLS-segment in GDT */
	if(thread_getTLSRange(cur,&tlsStart,&tlsEnd))
		gdt_setTLS(tlsStart,tlsEnd - tlsStart);
	else
		gdt_setTLS(0,0xFFFFFFFF);
	/* lock the FPU so that we can save the FPU-state for the previous process as soon
	 * as this one wants to use the FPU */
	fpu_lockFPU();
	thread_resume(cur->proc->pagedir.own,&cur->save,
			sll_length(cur->proc->threads) > 1 ? cur->kstackFrame : 0);
}


#if DEBUGGING

void thread_printState(const sThreadRegs *state) {
	vid_printf("\tState @ 0x%08Px:\n",state);
	vid_printf("\t\tesp = %#08x\n",state->esp);
	vid_printf("\t\tedi = %#08x\n",state->edi);
	vid_printf("\t\tesi = %#08x\n",state->esi);
	vid_printf("\t\tebp = %#08x\n",state->ebp);
	vid_printf("\t\teflags = %#08x\n",state->eflags);
}

#endif
