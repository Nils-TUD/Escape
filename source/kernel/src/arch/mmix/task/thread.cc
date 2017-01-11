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

#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <task/sched.h>
#include <task/smp.h>
#include <task/thread.h>
#include <task/timer.h>
#include <assert.h>
#include <common.h>
#include <config.h>
#include <cpu.h>
#include <errno.h>
#include <string.h>
#include <util.h>
#include <video.h>

/*
 * A few words to cloning and thread-switching on MMIX...
 * Unfortunatly, its a bit more complicated than on ECO32 and i586. There are two problems:
 * 1. The kernel-stack is accessed over the directly mapped space. That means, the address for
 *    every thread is different. Thus, we can't simply copy the kernel-stack, because the stack
 *    might contain pointers to the stack (which can't be distinguished from ordinary integers).
 * 2. It seems to be impossible to save the state somewhere and resume this state later. Because,
 *    when using SAVE the current stack is unusable before we've used UNSAVE to switch to another
 *    context. So, we can't continue after SAVE, but have to do an UNSAVE directly after the SAVE,
 *    i.e. the same assembler routine has to do that to ensure that the the stack is not accessed.
 *
 * The first problem can be solved by the following trick: When cloning, copy the current stack
 * to a temporary page. Then, leave the kernel directly (!) with the parent and make sure that the
 * parent gets a different stack next time when entering the kernel. As soon as the clone wants to
 * run, the stuff on the temporary page is copied to the page that the parent used for the stack
 * at the time we've cloned it. This way, pointers to the stack are no problem, because the same
 * stack-address is used by the clone.
 * Therefore, we introduce archAttr.tempStack to store the frame-number of the temporary stack for
 * a thread. When cloning, copy the stack to it and when the parent returns from thread_initSave,
 * switch the stack with the clone and leave the kernel. But there is still a problem: The
 * SAVE $X,1 at the beginning of a trap uses rSS for the kernel-stack. The corresponding UNSAVE 1,$X
 * uses rSS as well to know the beginning. Thus, we can't simply change rSS for the parent before
 * the UNSAVE 1,$X has been executed. Thus, we have to pass it in some way to the point after UNSAVE.
 * Fortunatly, our rK is always -1 in userspace, i.e. $255 can be directly set before the RESUME 1.
 * Therefore, $255 on the stack (created by SAVE 1,$X) can be used to hold the value of rSS.
 * So, finally, intrpt_forcedTrap does always set $255 on the stack to (2^63 | kstack * PAGE_SIZE),
 * which is put into rSS after UNSAVE.
 * => REALLY inconvenient and probably not very efficient. But I don't see a better solution...
 *
 * The second problem forces us to use a different thread-switch-mechanismn than in ECO32 and i586.
 * Because the old one relies on the opportunity to save the state at any place and resume it later.
 * Since we have to do the SAVE and the RESUME directly after another, we provide the function
 * Thread::doSwitchTo and use it when switching the thread. This is no problem in Thread::doSwitch()
 * and the initial state can be created by thread_initSave. This one is rather tricky as well. It
 * performs a SAVE at first to write the whole state to memory, copies the register- and software-
 * stack to newStack, writes rWW, ..., rZZ and the end of the stack to saveArea and executes an
 * UNSAVE from the previous SAVE. That means we're doing:
 * SAVE $255,0
 * ... copy ...
 * UNSAVE 0,$255
 * This way, the current stack remains usable and a copy has been created. Of course, we do more
 * than necessary, because most of the values wouldn't need to be restored. The only important ones
 * are rS and rO, which can't be set directly.
 */

extern void *stackCopy;
extern uint64_t stackCopySize;
Thread *Thread::cur = NULL;

uintptr_t ThreadBase::addInitialStack() {
	sassert(proc->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_POPULATE,NULL,0,stackRegions + 0) == 0);
	sassert(proc->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN | MAP_POPULATE,NULL,0,stackRegions + 1) == 0);
	return stackRegions[1]->virt();
}

int ThreadBase::initArch(Thread *t) {
	t->kstackFrame = PhysMem::allocate(PhysMem::KERN);
	if(t->kstackFrame == PhysMem::INVALID_FRAME)
		return -ENOMEM;
	t->tempStack = -1;
	t->intrptLevel = 0;
	return 0;
}

int ThreadBase::createArch(const Thread *src,Thread *dst,bool cloneProc) {
	dst->tempStack = -1;
	dst->kstackFrame = PhysMem::allocate(PhysMem::KERN);
	if(dst->kstackFrame == PhysMem::INVALID_FRAME)
		return -ENOMEM;
	if(!cloneProc) {
		/* add a new stack-region for the register-stack */
		int res = dst->getProc()->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
				MAP_STACK | MAP_GROWABLE | MAP_POPULATE,NULL,0,dst->stackRegions + 0);
		if(res < 0) {
			PhysMem::free(dst->kstackFrame,PhysMem::KERN);
			return res;
		}
		/* add a new stack-region for the software-stack */
		res = dst->getProc()->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
				MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN | MAP_POPULATE,NULL,0,dst->stackRegions + 1);
		if(res < 0) {
			/* remove register-stack */
			dst->getProc()->getVM()->unmap(dst->stackRegions[0]);
			dst->stackRegions[0] = NULL;
			PhysMem::free(dst->kstackFrame,PhysMem::KERN);
			return res;
		}
	}
	dst->intrptLevel = src->intrptLevel;
	memcpy(dst->specRegLevels,src->specRegLevels,sizeof(src->specRegLevels));
	memcpy(dst->userState,src->userState,sizeof(src->userState));
	return 0;
}

void ThreadBase::freeArch(Thread *t) {
	for(int i = 0; i < 2; i++) {
		if(t->stackRegions[i] != NULL) {
			t->getProc()->getVM()->unmap(t->stackRegions[i]);
			t->stackRegions[i] = NULL;
		}
	}
	if(t->tempStack != (frameno_t)-1) {
		PhysMem::free(t->tempStack,PhysMem::KERN);
		t->tempStack = -1;
	}
	PhysMem::free(t->kstackFrame,PhysMem::KERN);
}

int ThreadBase::finishClone(Thread *t,Thread *nt) {
	nt->tempStack = PhysMem::allocate(PhysMem::KERN);
	if(nt->tempStack == PhysMem::INVALID_FRAME)
		return -ENOMEM;
	int res = Thread::initSave(&nt->saveArea,(void*)(DIR_MAP_AREA | (nt->tempStack * PAGE_SIZE)));
	if(res == 0) {
		/* the parent needs a new kernel-stack for the next kernel-entry */
		/* switch stacks */
		frameno_t kstack = t->kstackFrame;
		t->kstackFrame = nt->kstackFrame;
		nt->kstackFrame = kstack;
	}
	return res;
}

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* copy stack of kernel-start */
	uint64_t *ssp,*rsp = (uint64_t*)(DIR_MAP_AREA | (nt->kstackFrame * PAGE_SIZE));
	uintptr_t start = (uintptr_t)rsp;
	memcpy(rsp,&stackCopy,stackCopySize);
	rsp += stackCopySize / sizeof(uint64_t) - 1;
	rsp[-8] = (uint64_t)&Thread::startup;							/* rJ = startup-code */
	/* store stack-end for UNSAVE */
	nt->saveArea.stackEnd = (uintptr_t)rsp;
	/* put arguments for the startup-code on the software-stack */
	ssp = (uint64_t*)(start + PAGE_SIZE - sizeof(uint64_t));
	*ssp-- = entryPoint;
	*ssp-- = (uint64_t)arg;
	*ssp = nt->getProc()->getEntryPoint();
	/* set sp and fp */
	rsp[-(13 + 1)] = (uint64_t)ssp;									/* $254 */
	rsp[-(13 + 2)] = (uint64_t)ssp;									/* $253 */
	/* no tempstack here */
	nt->tempStack = -1;
}

void ThreadBase::doSwitch() {
	Thread *old = Thread::getRunning();

	/* update runtime-stats */
	uint64_t cycles = CPU::rdtsc();
	uint64_t runtime = cycles - old->stats.cycleStart;
	old->stats.runtime += runtime;
	old->stats.curCycleCount += runtime;

	/* choose a new thread to run */
	Thread *n = Sched::perform(old,old->getCPU());
	n->stats.schedCount++;

	/* switch thread */
	if(EXPECT_TRUE(n->getTid() != old->getTid())) {
		setRunning(n);
		if(EXPECT_FALSE(PhysMem::shouldSetRegTimestamp()))
			VirtMem::setTimestamp(n,cycles);

		/* if we still have a temp-stack, copy the contents to our real stack and free the
		 * temp-stack */
		if(EXPECT_FALSE(n->tempStack != (frameno_t)-1)) {
			memcpy((void*)(DIR_MAP_AREA | n->kstackFrame * PAGE_SIZE),
					(void*)(DIR_MAP_AREA | n->tempStack * PAGE_SIZE),
					PAGE_SIZE);
			PhysMem::free(n->tempStack,PhysMem::KERN);
			n->tempStack = -1;
		}

		if(n->getProc()->getPageDir()->getAddrSpace()->getRefCount() > 1)
			Util::panic("Address space sharing not implemented");

		/* TODO we have to clear the TCs if the process shares its address-space with another one */
		SMP::schedule(n->getCPU(),n,cycles);
		n->stats.cycleStart = CPU::rdtsc();
		Thread::doSwitchTo(&old->saveArea,&n->saveArea,n->getProc()->getPageDir()->getRV(),n->getTid());
	}
	else {
		SMP::schedule(n->getCPU(),n,cycles);
		n->stats.cycleStart = CPU::rdtsc();
	}
}
