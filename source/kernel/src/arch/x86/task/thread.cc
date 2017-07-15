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

#include <arch/x86/gdt.h>
#include <mem/pagedir.h>
#include <task/proc.h>
#include <task/smp.h>
#include <task/thread.h>
#include <common.h>

static SpinLock switchLock;

int ThreadBase::initArch(Thread *t) {
	t->kernelStack = t->getProc()->getPageDir()->createKernelStack();
	t->fpuState = NULL;
	return 0;
}

uintptr_t ThreadBase::addInitialStack() {
	sassert(proc->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,stackRegions + 0) == 0);
	return stackRegions[0]->virt();
}

int ThreadBase::createArch(const Thread *src,Thread *dst,bool cloneProc) {
	PageTables::KStackAllocator alloc;
	if(cloneProc) {
		/* map the kernel-stack at the same address */
		if(dst->getProc()->getPageDir()->map(src->kernelStack,1,alloc,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
			return -ENOMEM;
		dst->kernelStack = src->kernelStack;
	}
	else {
		/* map the kernel-stack at a free address */
		dst->kernelStack = dst->getProc()->getPageDir()->createKernelStack();
		if(dst->kernelStack == 0)
			return -ENOMEM;

		if(~dst->getFlags() & T_IDLE) {
			/* add a new stack-region */
			int res = dst->getProc()->getVM()->map(NULL,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
					MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,dst->stackRegions + 0);
			if(res < 0) {
				dst->getProc()->getPageDir()->unmap(dst->kernelStack,1,alloc);
				return res;
			}
		}
	}
	FPU::cloneState(dst,src);
	return 0;
}

void ThreadBase::freeArch(Thread *t) {
	if(t->stackRegions[0] != NULL) {
		t->getProc()->getVM()->unmap(t->stackRegions[0]);
		t->stackRegions[0] = NULL;
	}
	t->getProc()->getPageDir()->removeKernelStack(t->kernelStack);
	FPU::freeState(t);
}

int ThreadBase::finishClone(Thread *t,Thread *nt) {
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	frameno_t frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	ulong *dst = (ulong*)PageDir::getAccess(frame);

	if(Thread::save(&nt->saveArea)) {
		/* child */
		return 1;
	}

	/* we don't need to copy the whole page. but take into account that we call Thread::save, which
	 * internally does a push, so start 8 bytes earlier */
	ulong sp = CPU::getSP() - 8;
	ulong *src = (ulong*)t->kernelStack;
	size_t off = (sp & (PAGE_SIZE - 1)) / sizeof(ulong);
	ulong *dstend = dst + (PAGE_SIZE / sizeof(ulong)) - 1;
	dst += off;
	src += off;
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	while(dst < dstend)
		*dst++ = *src++;
	/* store thread at the top */
	*dst = (ulong)nt;

	PageDir::removeAccess(frame);
	/* parent */
	return 0;
}

void ThreadBase::finishThreadStart(A_UNUSED Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint) {
	/* setup kernel-stack */
	frameno_t frame = nt->getProc()->getPageDir()->getFrameNo(nt->kernelStack);
	ulong *dst = (ulong*)PageDir::getAccess(frame);
	ulong sp = nt->kernelStack + PAGE_SIZE - sizeof(ulong) * 6;
	dst += (PAGE_SIZE / sizeof(ulong)) - 1;
	*dst = (ulong)nt;
	*--dst = nt->getProc()->getEntryPoint();
	*--dst = entryPoint;
	*--dst = (ulong)arg;
	*--dst = (ulong)&Thread::startup;
	*--dst = sp;
	PageDir::removeAccess(frame);

	/* prepare registers for the first Thread::resume() */
	nt->saveArea.prepareStart(sp);
}

void Thread::initialSwitch() {
	switchLock.down();
	cpuid_t cpu = GDT::getCPUId();
	Thread *cur = Sched::perform(NULL,cpu);
	cur->stats.schedCount++;
	if(PhysMem::shouldSetRegTimestamp())
		VirtMem::setTimestamp(cur,Timer::getRuntime());
	GDT::prepareRun(cpu,true,cur);
	cur->setCPU(cpu);
	FPU::lockFPU();
	cur->stats.cycleStart = CPU::rdtsc();
	Thread::resume(cur->getProc()->getPageDir()->getPhysAddr(),&cur->saveArea,&switchLock,true);
}

void ThreadBase::doSwitch() {
	Thread *old = Thread::getRunning();
	/* lock this, because Sched::perform() may make us ready and we can't be chosen by another CPU
	 * until we've really switched the thread (kernelstack, ...) */
	switchLock.down();

	/* update runtime-stats */
	uint64_t cycles = CPU::rdtsc();
	uint64_t runtime = cycles - old->stats.cycleStart;
	old->stats.runtime += runtime;
	old->stats.curCycleCount += runtime;
	cpuid_t cpu = old->getCPU();

	/* choose a new thread to run */
	Thread *n = Sched::perform(old,cpu);
	n->stats.schedCount++;

	/* switch thread */
	if(EXPECT_TRUE(n->getTid() != old->getTid())) {
		if(EXPECT_FALSE(PhysMem::shouldSetRegTimestamp()))
			VirtMem::setTimestamp(n,cycles);
		GDT::prepareRun(cpu,n->getProc() != old->getProc(),n);
		if(cpu != n->getCPU()) {
			FPU::initSaveState(n);
			n->getStats().migrations++;
		}
		n->setCPU(cpu);

		/* some stats for SMP */
		SMP::schedule(cpu,n,cycles);

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		FPU::lockFPU();

		n->stats.cycleStart = CPU::rdtsc();
		uintptr_t pdir = n->getProc()->getPageDir()->getPhysAddr();
		bool chgpdir = n->getProc() != old->getProc();
		if(!Thread::save(&old->saveArea)) {
			/* old thread */
			Thread::resume(pdir,&n->saveArea,&switchLock,chgpdir);
		}
	}
	else {
		SMP::schedule(cpu,n,cycles);
		n->stats.cycleStart = CPU::rdtsc();
		switchLock.up();
	}
}
