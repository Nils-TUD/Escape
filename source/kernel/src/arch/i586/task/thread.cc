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
#include <sys/arch/i586/gdt.h>
#include <sys/arch/i586/fpu.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/config.h>
#include <sys/cpu.h>
#include <assert.h>
#include <errno.h>

EXTERN_C void thread_startup(void);
EXTERN_C bool thread_save(sThreadRegs *saveArea);
EXTERN_C bool thread_resume(uintptr_t pageDir,const sThreadRegs *saveArea,klock_t *lock,bool newProc);

static klock_t switchLock;
static bool threadSet = false;

int ThreadBase::initArch(Thread *t) {
	t->kernelStack = t->getProc()->getPageDir()->createKernelStack();
	t->fpuState = NULL;
	return 0;
}

void ThreadBase::addInitialStack() {
	int res;
	assert(tid == INIT_TID);
	res = proc->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,stackRegions + 0);
	assert(res == 0);
}

int ThreadBase::createArch(const Thread *src,Thread *dst,bool cloneProc) {
	if(cloneProc) {
		/* map the kernel-stack at the same address */
		if(dst->getProc()->getPageDir()->map(src->kernelStack,NULL,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
			return -ENOMEM;
		dst->kernelStack = src->kernelStack;
	}
	else {
		int res;
		/* map the kernel-stack at a free address */
		dst->kernelStack = dst->getProc()->getPageDir()->createKernelStack();
		if(dst->kernelStack == 0)
			return -ENOMEM;

		/* add a new stack-region */
		res = dst->getProc()->getVM()->map(0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
				MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,dst->stackRegions + 0);
		if(res < 0) {
			dst->getProc()->getPageDir()->unmap(dst->kernelStack,1,true);
			return res;
		}
	}
	FPU::cloneState(&(dst->fpuState),src->fpuState);
	return 0;
}

void ThreadBase::freeArch(Thread *t) {
	if(t->stackRegions[0] != NULL) {
		t->getProc()->getVM()->remove(t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	t->getProc()->getPageDir()->removeKernelStack(t->kernelStack);
	FPU::freeState(&t->fpuState);
}

int ThreadBase::finishClone(Thread *t,Thread *nt) {
	ulong *src,*dst;
	size_t i;
	frameno_t frame;
	/* ensure that we won't get interrupted */
	klock_t lock = 0;
	spinlock_aquire(&lock);
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	dst = (ulong*)PageDir::mapToTemp(&frame,1);

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (ulong*)t->kernelStack;
	for(i = 0; i < PT_ENTRY_COUNT - 1; i++)
		*dst++ = *src++;
	/* store thread at the top */
	*dst = (ulong)nt;

	PageDir::unmapFromTemp(1);
	spinlock_release(&lock);
	/* parent */
	return 0;
}

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* setup kernel-stack */
	frameno_t frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	ulong *dst = (ulong*)PageDir::mapToTemp(&frame,1);
	uint32_t sp = nt->kernelStack + PAGE_SIZE - sizeof(int) * 6;
	dst += PT_ENTRY_COUNT - 1;
	*dst = (uint32_t)nt;
	*--dst = nt->getProc()->getEntryPoint();
	*--dst = entryPoint;
	*--dst = (ulong)arg;
	*--dst = (ulong)&thread_startup;
	*--dst = sp;
	PageDir::unmapFromTemp(1);

	/* prepare registers for the first thread_resume() */
	nt->save.ebp = sp;
	nt->save.esp = sp;
	nt->save.ebx = 0;
	nt->save.edi = 0;
	nt->save.esi = 0;
	nt->save.eflags = 0;
}

bool ThreadBase::beginTerm() {
	bool res;
	spinlock_aquire(&switchLock);
	/* at first the thread can't run to do that. if its not running, its important that no resources
	 * or heap-allocations are hold. otherwise we would produce a deadlock or memory-leak */
	res = state != Thread::RUNNING && termHeapCount == 0 && !hasResources();
	/* ensure that the thread won't be chosen again */
	if(res)
		Sched::removeThread(static_cast<Thread*>(this));
	spinlock_release(&switchLock);
	return res;
}

void Thread::initialSwitch() {
	Thread *cur;
	spinlock_aquire(&switchLock);
	cur = Sched::perform(NULL,0);
	cur->stats.schedCount++;
	VirtMem::setTimestamp(cur,Timer::getTimestamp());
	cur->setCPU(gdt_prepareRun(NULL,cur));
	FPU::lockFPU();
	cur->stats.cycleStart = CPU::rdtsc();
	thread_resume(cur->getProc()->getPageDir()->getPhysAddr(),&cur->save,&switchLock,true);
}

void ThreadBase::doSwitch() {
	uint64_t cycles,runtime;
	Thread *old = Thread::getRunning();
	Thread *n;
	/* lock this, because sched_perform() may make us ready and we can't be chosen by another CPU
	 * until we've really switched the thread (kernelstack, ...) */
	spinlock_aquire(&switchLock);

	/* update runtime-stats */
	cycles = CPU::rdtsc();
	runtime = Timer::cyclesToTime(cycles - old->stats.cycleStart);
	old->stats.runtime += runtime;
	old->stats.curCycleCount += cycles - old->stats.cycleStart;

	/* choose a new thread to run */
	n = Sched::perform(old,runtime);
	n->stats.schedCount++;

	/* switch thread */
	if(n->getTid() != old->getTid()) {
		time_t timestamp = Timer::cyclesToTime(cycles);
		VirtMem::setTimestamp(n,timestamp);
		n->setCPU(gdt_prepareRun(old,n));

		/* some stats for SMP */
		SMP::schedule(n->getCPU(),n,timestamp);

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		FPU::lockFPU();
		if(!thread_save(&old->save)) {
			/* old thread */
			n->stats.cycleStart = CPU::rdtsc();
			thread_resume(n->getProc()->getPageDir()->getPhysAddr(),&n->save,&switchLock,
			              n->getProc() != old->getProc());
		}
	}
	else {
		n->stats.cycleStart = CPU::rdtsc();
		spinlock_release(&switchLock);
	}
}

#if DEBUGGING

void ThreadBase::printState(const sThreadRegs *state) {
	vid_printf("State @ 0x%08Px:\n",state);
	vid_printf("\tesp = %#08x\n",state->esp);
	vid_printf("\tedi = %#08x\n",state->edi);
	vid_printf("\tesi = %#08x\n",state->esi);
	vid_printf("\tebp = %#08x\n",state->ebp);
	vid_printf("\teflags = %#08x\n",state->eflags);
}

#endif
