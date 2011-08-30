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
#include <sys/task/smp.h>
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <sys/config.h>
#include <sys/cpu.h>
#include <assert.h>
#include <errors.h>

#define KSTACK_CURTHREAD_ADDR	(KERNEL_STACK + PAGE_SIZE - sizeof(int))

extern void thread_startup(void);
extern bool thread_save(sThreadRegs *saveArea);
extern bool thread_resume(uintptr_t pageDir,const sThreadRegs *saveArea,klock_t *lock);

static klock_t switchLock;
static bool threadSet = false;

int thread_initArch(sThread *t) {
	t->archAttr.kernelStack = paging_createKernelStack(&t->proc->pagedir);
	t->archAttr.fpuState = NULL;
	return 0;
}

void thread_addInitialStack(sThread *t) {
	assert(t->tid == INIT_TID);
	t->stackRegions[0] = vmm_add(t->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
	assert(t->stackRegions[0] >= 0);
}

int thread_createArch(const sThread *src,sThread *dst,bool cloneProc) {
	if(cloneProc) {
		/* map the kernel-stack at the same address */
		paging_mapTo(&dst->proc->pagedir,src->archAttr.kernelStack,NULL,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
		dst->archAttr.kernelStack = src->archAttr.kernelStack;
	}
	else {
		if(pmem_getFreeFrames(MM_DEF) < INITIAL_STACK_PAGES)
			return ERR_NOT_ENOUGH_MEM;

		/* map the kernel-stack at a free address */
		dst->archAttr.kernelStack = paging_createKernelStack(&dst->proc->pagedir);

		/* add a new stack-region */
		dst->stackRegions[0] = vmm_add(dst->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
		if(dst->stackRegions[0] < 0) {
			paging_unmapFrom(&dst->proc->pagedir,dst->archAttr.kernelStack,1,true);
			return dst->stackRegions[0];
		}
	}
	fpu_cloneState(&(dst->archAttr.fpuState),src->archAttr.fpuState);
	return 0;
}

void thread_freeArch(sThread *t) {
	if(t->stackRegions[0] >= 0) {
		vmm_remove(t->proc->pid,t->stackRegions[0]);
		t->stackRegions[0] = -1;
	}
	paging_unmapFrom(&t->proc->pagedir,t->archAttr.kernelStack,1,true);
	fpu_freeState(&t->archAttr.fpuState);
}

int thread_finishClone(sThread *t,sThread *nt) {
	UNUSED(t);
	ulong *src;
	size_t i;
	/* ensure that we won't get interrupted */
	klock_t lock = 0;
	klock_aquire(&lock);
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	frameno_t frame = paging_getFrameNo(&nt->proc->pagedir,nt->archAttr.kernelStack);
	ulong *dst = (ulong*)paging_mapToTemp(&frame,1);

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (ulong*)t->archAttr.kernelStack;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	paging_unmapFromTemp(1);
	klock_release(&lock);
	/* parent */
	return 0;
}

void thread_finishThreadStart(sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint) {
	UNUSED(t);
	/* setup kernel-stack */
	frameno_t frame = paging_getFrameNo(&nt->proc->pagedir,nt->archAttr.kernelStack);
	ulong *dst = (ulong*)paging_mapToTemp(&frame,1);
	uint32_t sp = nt->archAttr.kernelStack + PAGE_SIZE - sizeof(int) * 6;
	dst += PT_ENTRY_COUNT - 1;
	*--dst = nt->proc->entryPoint;
	*--dst = entryPoint;
	*--dst = (ulong)arg;
	*--dst = (ulong)&thread_startup;
	*--dst = sp;
	paging_unmapFromTemp(1);

	/* prepare registers for the first thread_resume() */
	nt->save.ebp = sp;
	nt->save.esp = sp;
	nt->save.ebx = 0;
	nt->save.edi = 0;
	nt->save.esi = 0;
	nt->save.eflags = 0;
}

sThread *thread_getRunning(void) {
	return threadSet ? gdt_getRunning() : NULL;
}

void thread_setRunning(sThread *t) {
	gdt_setRunning(gdt_getCPUId(),t);
	threadSet = true;
}

void thread_initialSwitch(void) {
	sThread *cur;
	klock_aquire(&switchLock);
	cur = sched_perform(NULL);
	cur->stats.schedCount++;
	if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
		vmm_setTimestamp(cur,timer_getTimestamp());
	cur->cpu = gdt_prepareRun(NULL,cur);
	fpu_lockFPU();
	cur->stats.cycleStart = cpu_rdtsc();
	thread_resume(cur->proc->pagedir.own,&cur->save,&switchLock);
}

void thread_doSwitch(void) {
	sThread *old = thread_getRunning();
	sThread *new;
	/* lock this, because sched_perform() may make us ready and we can't be chosen by another CPU
	 * until we've really switched the thread (kernelstack, ...) */
	klock_aquire(&switchLock);
	new = sched_perform(old);
	if(new->tid != old->tid) {
		/* update stats */
		uint64_t cycles = cpu_rdtsc();
		time_t timestamp = timer_getTimestamp();
		assert(old->stats.cycleStart != 0);
		old->stats.runtime += timer_cyclesToTime(cycles - old->stats.cycleStart);
		old->stats.curCycleCount += cycles - old->stats.cycleStart;
		new->stats.schedCount++;

		if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
			vmm_setTimestamp(new,timestamp);
		new->cpu = gdt_prepareRun(old,new);

		/* some stats for SMP */
		if(!(old->flags & T_IDLE))
			smp_unschedule(old->cpu,timestamp);
		if(!(new->flags & T_IDLE))
			smp_schedule(new->cpu,timestamp);

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		fpu_lockFPU();
		if(!thread_save(&old->save)) {
			/* old thread */
			new->stats.cycleStart = cpu_rdtsc();
			thread_resume(new->proc->pagedir.own,&new->save,&switchLock);
		}
	}
	else
		klock_release(&switchLock);

	proc_killDeadThread();
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
