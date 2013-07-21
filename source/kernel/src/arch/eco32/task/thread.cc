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
#include <sys/task/proc.h>
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

Thread *Thread::cur = NULL;

EXTERN_C void thread_startup(void);
EXTERN_C bool thread_save(sThreadRegs *saveArea);
EXTERN_C bool thread_resume(pagedir_t pageDir,const sThreadRegs *saveArea,frameno_t kstackFrame);

void ThreadBase::addInitialStack() {
	assert(tid == INIT_TID);
	assert(proc->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWSDOWN | MAP_GROWABLE,NULL,0,stackRegions + 0) == 0);
}

int ThreadBase::initArch(Thread *t) {
	/* setup kernel-stack for us */
	frameno_t stackFrame = PhysMem::allocate(PhysMem::KERN);
	if(stackFrame == 0)
		return -ENOMEM;
	if(paging_mapTo(t->getProc()->getPageDir(),KERNEL_STACK,&stackFrame,1,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0) {
		PhysMem::free(stackFrame,PhysMem::KERN);
		return -ENOMEM;
	}
	t->kstackFrame = stackFrame;
	return 0;
}

int ThreadBase::createArch(A_UNUSED const Thread *src,Thread *dst,bool cloneProc) {
	if(cloneProc) {
		frameno_t stackFrame = PhysMem::allocate(PhysMem::KERN);
		if(stackFrame == 0)
			return -ENOMEM;
		if(paging_mapTo(dst->getProc()->getPageDir(),KERNEL_STACK,&stackFrame,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0) {
			PhysMem::free(stackFrame,PhysMem::KERN);
			return -ENOMEM;
		}
		dst->kstackFrame = stackFrame;
	}
	else {
		int res;
		dst->kstackFrame = PhysMem::allocate(PhysMem::KERN);
		if(dst->kstackFrame == 0)
			return -ENOMEM;

		/* add a new stack-region */
		res = dst->getProc()->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,
				PROT_READ | PROT_WRITE,MAP_STACK | MAP_GROWSDOWN | MAP_GROWABLE,NULL,0,
				dst->stackRegions + 0);
		if(res < 0) {
			PhysMem::free(dst->kstackFrame,PhysMem::KERN);
			return res;
		}
	}
	return 0;
}

void ThreadBase::freeArch(Thread *t) {
	if(t->stackRegions[0] != NULL) {
		t->getProc()->getVM()->remove(t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	PhysMem::free(t->kstackFrame,PhysMem::KERN);
}

int ThreadBase::finishClone(A_UNUSED Thread *t,Thread *nt) {
	ulong *src;
	size_t i;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	ulong *dst = (ulong*)(DIR_MAPPED_SPACE | (nt->kstackFrame * PAGE_SIZE));

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

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* prepare registers for the first thread_resume() */
	nt->save.r16 = nt->getProc()->getEntryPoint();
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

bool ThreadBase::beginTerm() {
	/* at first the thread can't run to do that. if its not running, its important that no resources
	 * or heap-allocations are hold. otherwise we would produce a deadlock or memory-leak */
	bool res = getState() != Thread::RUNNING && termHeapCount == 0 && !hasResources();
	/* ensure that the thread won't be chosen again */
	if(res)
		Sched::removeThread(static_cast<Thread*>(this));
	return res;
}

void ThreadBase::doSwitch(void) {
	Thread *old = Thread::getRunning();
	Thread *n;
	/* eco32 has no cycle-counter or similar. therefore we use the timer for runtime-
	 * measurement */
	time_t timestamp = Timer::getTimestamp();
	uint64_t runtime = (timestamp - old->stats.cycleStart) * 1000;
	old->stats.runtime += runtime;
	old->stats.curCycleCount += timestamp - old->stats.cycleStart;

	/* choose a new thread to run */
	n = Sched::perform(old,runtime);
	n->stats.schedCount++;

	if(n->getTid() != old->getTid()) {
		if(!thread_save(&old->save)) {
			setRunning(n);
			VirtMem::setTimestamp(n,timestamp);

			SMP::schedule(n->getCPU(),n,timestamp);
			n->stats.cycleStart = timestamp;
			thread_resume(*n->getProc()->getPageDir(),&n->save,n->kstackFrame);
		}
	}
	else
		n->stats.cycleStart = timestamp;
}


#if DEBUGGING

void ThreadBase::printState(const sThreadRegs *state) {
	vid_printf("State:\n",state);
	vid_printf("\t$16 = %#08x\n",state->r16);
	vid_printf("\t$17 = %#08x\n",state->r17);
	vid_printf("\t$18 = %#08x\n",state->r18);
	vid_printf("\t$19 = %#08x\n",state->r19);
	vid_printf("\t$20 = %#08x\n",state->r20);
	vid_printf("\t$21 = %#08x\n",state->r21);
	vid_printf("\t$22 = %#08x\n",state->r22);
	vid_printf("\t$23 = %#08x\n",state->r23);
	vid_printf("\t$29 = %#08x\n",state->r29);
	vid_printf("\t$30 = %#08x\n",state->r30);
	vid_printf("\t$31 = %#08x\n",state->r31);
}

#endif
