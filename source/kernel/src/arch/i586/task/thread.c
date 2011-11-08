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
#include <sys/spinlock.h>
#include <sys/config.h>
#include <sys/cpu.h>
#include <assert.h>
#include <errno.h>

extern void thread_startup(void);
extern bool thread_save(sThreadRegs *saveArea);
extern bool thread_resume(uintptr_t pageDir,const sThreadRegs *saveArea,klock_t *lock,bool newProc);

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

size_t thread_getThreadFrmCnt(void) {
	return INITIAL_STACK_PAGES;
}

int thread_createArch(const sThread *src,sThread *dst,bool cloneProc) {
	if(cloneProc) {
		/* map the kernel-stack at the same address */
		if(paging_mapTo(&dst->proc->pagedir,src->archAttr.kernelStack,NULL,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
			return -ENOMEM;
		dst->archAttr.kernelStack = src->archAttr.kernelStack;
	}
	else {
		/* map the kernel-stack at a free address */
		dst->archAttr.kernelStack = paging_createKernelStack(&dst->proc->pagedir);
		if(dst->archAttr.kernelStack == 0)
			return -ENOMEM;

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
	ulong *src,*dst;
	size_t i;
	frameno_t frame;
	/* ensure that we won't get interrupted */
	klock_t lock = 0;
	spinlock_aquire(&lock);
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	frame = paging_getFrameNo(&nt->proc->pagedir,nt->archAttr.kernelStack);
	dst = (ulong*)paging_mapToTemp(&frame,1);

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
	spinlock_release(&lock);
	/* parent */
	return 0;
}

void thread_finishThreadStart(A_UNUSED sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint) {
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

uint64_t thread_getRuntime(const sThread *t) {
	if(t->state == ST_RUNNING) {
		/* if the thread is running, we must take the time since the last scheduling of that thread
		 * into account. this is especially a problem with idle-threads */
		uint64_t cycles = cpu_rdtsc();
		return (t->stats.runtime + timer_cyclesToTime(cycles - t->stats.cycleStart));
	}
	return t->stats.runtime;
}

sThread *thread_getRunning(void) {
	if(threadSet)
		return gdt_getRunning();
	return NULL;
}

void thread_setRunning(sThread *t) {
	gdt_setRunning(gdt_getCPUId(),t);
	threadSet = true;
}

bool thread_beginTerm(sThread *t) {
	bool res;
	spinlock_aquire(&switchLock);
	/* at first the thread can't run to do that. if its not running, its important that no resources
	 * or heap-allocations are hold. otherwise we would produce a deadlock or memory-leak */
	res = t->state != ST_RUNNING && t->termHeapCount == 0 && t->resources == 0;
	/* ensure that the thread won't be chosen again */
	if(res)
		sched_removeThread(t);
	spinlock_release(&switchLock);
	return res;
}

void thread_initialSwitch(void) {
	sThread *cur;
	spinlock_aquire(&switchLock);
	cur = sched_perform(NULL,0);
	cur->stats.schedCount++;
	if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
		vmm_setTimestamp(cur,timer_getTimestamp());
	cur->cpu = gdt_prepareRun(NULL,cur);
	fpu_lockFPU();
	cur->stats.cycleStart = cpu_rdtsc();
	thread_resume(cur->proc->pagedir.own,&cur->save,&switchLock,true);
}

void thread_doSwitch(void) {
	uint64_t cycles,runtime;
	sThread *old = thread_getRunning();
	sThread *new;
	/* lock this, because sched_perform() may make us ready and we can't be chosen by another CPU
	 * until we've really switched the thread (kernelstack, ...) */
	spinlock_aquire(&switchLock);

	/* update runtime-stats */
	cycles = cpu_rdtsc();
	runtime = timer_cyclesToTime(cycles - old->stats.cycleStart);
	old->stats.runtime += runtime;
	old->stats.curCycleCount += cycles - old->stats.cycleStart;

	/* choose a new thread to run */
	new = sched_perform(old,runtime);
	new->stats.schedCount++;

	/* switch thread */
	if(new->tid != old->tid) {
		time_t timestamp = timer_cyclesToTime(cycles);
		if(conf_getStr(CONF_SWAP_DEVICE) != NULL)
			vmm_setTimestamp(new,timestamp);
		new->cpu = gdt_prepareRun(old,new);

		/* some stats for SMP */
		smp_schedule(new->cpu,new,timestamp);

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		fpu_lockFPU();
		if(!thread_save(&old->save)) {
			/* old thread */
			new->stats.cycleStart = cpu_rdtsc();
			thread_resume(new->proc->pagedir.own,&new->save,&switchLock,new->proc != old->proc);
		}
	}
	else {
		new->stats.cycleStart = cpu_rdtsc();
		spinlock_release(&switchLock);
	}
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
