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
#include <sys/mem/vmm.h>
#include <sys/mem/paging.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <errno.h>

extern void thread_startup(void);
extern bool thread_save(sThreadRegs *saveArea);
extern bool thread_resume(pagedir_t pageDir,const sThreadRegs *saveArea,frameno_t kstackFrame);

static sThread *cur = NULL;

int thread_initArch(sThread *t) {
	/* setup kernel-stack for us */
	frameno_t stackFrame = pmem_allocate(FRM_KERNEL);
	if(stackFrame == 0)
		return -ENOMEM;
	if(paging_mapTo(&t->proc->pagedir,KERNEL_STACK,&stackFrame,1,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0) {
		pmem_free(stackFrame,FRM_KERNEL);
		return -ENOMEM;
	}
	t->archAttr.kstackFrame = stackFrame;
	return 0;
}

void thread_addInitialStack(sThread *t) {
	assert(t->tid == INIT_TID);
	assert(vmm_add(t->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
			INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK,t->stackRegions + 0,0) == 0);
}

size_t thread_getThreadFrmCnt(void) {
	return INITIAL_STACK_PAGES;
}

int thread_createArch(A_UNUSED const sThread *src,sThread *dst,bool cloneProc) {
	if(cloneProc) {
		frameno_t stackFrame = pmem_allocate(FRM_KERNEL);
		if(stackFrame == 0)
			return -ENOMEM;
		if(paging_mapTo(&dst->proc->pagedir,KERNEL_STACK,&stackFrame,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0) {
			pmem_free(stackFrame,FRM_KERNEL);
			return -ENOMEM;
		}
		dst->archAttr.kstackFrame = stackFrame;
	}
	else {
		int res;
		dst->archAttr.kstackFrame = pmem_allocate(FRM_KERNEL);
		if(dst->archAttr.kstackFrame == 0)
			return -ENOMEM;

		/* add a new stack-region */
		res = vmm_add(dst->proc->pid,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK,dst->stackRegions + 0,0);
		if(res < 0) {
			pmem_free(dst->archAttr.kstackFrame,FRM_KERNEL);
			return res;
		}
	}
	return 0;
}

void thread_freeArch(sThread *t) {
	if(t->stackRegions[0] != NULL) {
		vmm_remove(t->proc->pid,t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	pmem_free(t->archAttr.kstackFrame,FRM_KERNEL);
}

sThread *thread_getRunning(void) {
	return cur;
}

void thread_setRunning(sThread *t) {
	cur = t;
}

int thread_finishClone(A_UNUSED sThread *t,sThread *nt) {
	ulong *src;
	size_t i;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	ulong *dst = (ulong*)(DIR_MAPPED_SPACE | (nt->archAttr.kstackFrame * PAGE_SIZE));

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (ulong*)KERNEL_STACK;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	/* parent */
	return 0;
}

void thread_finishThreadStart(A_UNUSED sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint) {
	/* prepare registers for the first thread_resume() */
	nt->save.r16 = nt->proc->entryPoint;
	nt->save.r17 = entryPoint;
	nt->save.r18 = (uint32_t)arg;
	nt->save.r19 = 0;
	nt->save.r20 = 0;
	nt->save.r21 = 0;
	nt->save.r22 = 0;
	nt->save.r23 = 0;
	nt->save.r25 = 0;
	nt->save.r29 = KERNEL_STACK + PAGE_SIZE - sizeof(int);
	nt->save.r30 = 0;
	nt->save.r31 = (uint32_t)&thread_startup;
}

uint64_t thread_getRuntime(const sThread *t) {
	return t->stats.runtime;
}

bool thread_beginTerm(sThread *t) {
	/* at first the thread can't run to do that. if its not running, its important that no resources
	 * or heap-allocations are hold. otherwise we would produce a deadlock or memory-leak */
	bool res = t->state != ST_RUNNING && t->termHeapCount == 0 && t->resources == 0;
	/* ensure that the thread won't be chosen again */
	if(res)
		sched_removeThread(t);
	return res;
}

void thread_doSwitch(void) {
	sThread *old = thread_getRunning();
	sThread *new;
	/* eco32 has no cycle-counter or similar. therefore we use the timer for runtime-
	 * measurement */
	time_t timestamp = timer_getTimestamp();
	uint64_t runtime = (timestamp - old->stats.cycleStart) * 1000;
	old->stats.runtime += runtime;

	/* choose a new thread to run */
	new = sched_perform(old,runtime);
	new->stats.schedCount++;

	if(new->tid != old->tid) {
		if(!thread_save(&old->save)) {
			thread_setRunning(new);
			vmm_setTimestamp(new,timestamp);

			smp_schedule(new->cpu,new,timestamp);
			new->stats.cycleStart = timestamp;
			thread_resume(new->proc->pagedir,&new->save,new->archAttr.kstackFrame);
		}
	}
	else
		new->stats.cycleStart = timestamp;
}


#if DEBUGGING

void thread_printState(const sThreadRegs *state) {
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
