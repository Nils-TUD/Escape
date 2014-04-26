/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/mem/virtmem.h>
#include <sys/mem/pagedir.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/video.h>
#include <assert.h>
#include <errno.h>

Thread *Thread::cur = NULL;

void ThreadBase::addInitialStack() {
	assert(tid == INIT_TID);
	assert(proc->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWSDOWN | MAP_GROWABLE,NULL,0,stackRegions + 0) == 0);
}

int ThreadBase::initArch(Thread *t) {
	/* setup kernel-stack for us */
	frameno_t stackFrame = PhysMem::allocate(PhysMem::KERN);
	if(stackFrame == 0)
		return -ENOMEM;
	if(t->getProc()->getPageDir()->map(KERNEL_STACK,&stackFrame,1,
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
		if(dst->getProc()->getPageDir()->map(KERNEL_STACK,&stackFrame,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0) {
			PhysMem::free(stackFrame,PhysMem::KERN);
			return -ENOMEM;
		}
		dst->kstackFrame = stackFrame;
	}
	else {
		dst->kstackFrame = PhysMem::allocate(PhysMem::KERN);
		if(dst->kstackFrame == 0)
			return -ENOMEM;

		/* add a new stack-region */
		int res = dst->getProc()->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,
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
		t->getProc()->getVM()->unmap(t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	PhysMem::free(t->kstackFrame,PhysMem::KERN);
}

int ThreadBase::finishClone(A_UNUSED Thread *t,Thread *nt) {
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	ulong *dst = (ulong*)(DIR_MAPPED_SPACE | (nt->kstackFrame * PAGE_SIZE));

	if(Thread::save(&nt->saveArea)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	ulong *src = (ulong*)KERNEL_STACK;
	for(size_t i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	/* parent */
	return 0;
}

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* prepare registers for the first Thread::resume() */
	nt->saveArea.r16 = nt->getProc()->getEntryPoint();
	nt->saveArea.r17 = entryPoint;
	nt->saveArea.r18 = (uint32_t)arg;
	nt->saveArea.r19 = 0;
	nt->saveArea.r20 = 0;
	nt->saveArea.r21 = 0;
	nt->saveArea.r22 = 0;
	nt->saveArea.r23 = 0;
	nt->saveArea.r25 = 0;
	nt->saveArea.r29 = KERNEL_STACK + PAGE_SIZE - sizeof(int);
	nt->saveArea.r30 = 0;
	nt->saveArea.r31 = (uint32_t)&Thread::startup;
}

void ThreadBase::doSwitch() {
	Thread *old = Thread::getRunning();
	/* eco32 has no cycle-counter or similar. therefore we use the timer for runtime-
	 * measurement */
	uint64_t cycles = CPU::rdtsc();
	uint64_t runtime = cycles - old->stats.cycleStart;
	old->stats.runtime += runtime;
	old->stats.curCycleCount += runtime;

	/* choose a new thread to run */
	Thread *n = Sched::perform(old,old->getCPU());
	n->stats.schedCount++;

	if(EXPECT_TRUE(n->getTid() != old->getTid())) {
		if(!Thread::save(&old->saveArea)) {
			setRunning(n);
			if(EXPECT_FALSE(PhysMem::shouldSetRegTimestamp()))
				VirtMem::setTimestamp(n,cycles);

			SMP::schedule(n->getCPU(),n,cycles);
			n->stats.cycleStart = CPU::rdtsc();
			Thread::resume(n->getProc()->getPageDir()->getPhysAddr(),&n->saveArea,n->kstackFrame);
		}
	}
	else {
		SMP::schedule(n->getCPU(),n,cycles);
		n->stats.cycleStart = CPU::rdtsc();
	}
}

void ThreadBase::printState(OStream &os,const ThreadRegs *st) const {
	os.writef("State:\n",st);
	os.writef("\t$16 = %#08x\n",st->r16);
	os.writef("\t$17 = %#08x\n",st->r17);
	os.writef("\t$18 = %#08x\n",st->r18);
	os.writef("\t$19 = %#08x\n",st->r19);
	os.writef("\t$20 = %#08x\n",st->r20);
	os.writef("\t$21 = %#08x\n",st->r21);
	os.writef("\t$22 = %#08x\n",st->r22);
	os.writef("\t$23 = %#08x\n",st->r23);
	os.writef("\t$29 = %#08x\n",st->r29);
	os.writef("\t$30 = %#08x\n",st->r30);
	os.writef("\t$31 = %#08x\n",st->r31);
}
