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

#pragma once

#include <sys/common.h>
#include <sys/cpu.h>

class Thread : public ThreadBase {
	friend class ThreadBase;
public:
	static void pushSpecRegs();
	static void popSpecRegs();
	sKSpecRegs *getSpecRegs() const;

	/**
	 * @return the frame mapped at KERNEL_STACK
	 */
	frameno_t getKernelStack() const {
		return kstackFrame;
	}

private:
	/* the frame mapped at KERNEL_STACK */
	frameno_t kstackFrame;
	/* use as a temporary kernel-stack for cloning */
	frameno_t tempStack;
	/* when handling a signal, we have to backup these registers */
	sKSpecRegs specRegLevels[MAX_INTRPT_LEVELS];
	static Thread *cur;
};

inline void ThreadBase::addInitialStack() {
	assert(tid == INIT_TID);
	assert(vmm_map(proc->pid,0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE,NULL,0,stackRegions + 0) == 0);
	assert(vmm_map(proc->pid,0,INITIAL_STACK_PAGES * PAGE_SIZE,0,PROT_READ | PROT_WRITE,
			MAP_STACK | MAP_GROWABLE | MAP_GROWSDOWN,NULL,0,stackRegions + 1) == 0);
}

inline size_t ThreadBase::getThreadFrmCnt() {
	return INITIAL_STACK_PAGES * 2;
}

inline Thread *ThreadBase::getRunning() {
	return Thread::cur;
}

inline void ThreadBase::setRunning(Thread *t) {
	Thread::cur = t;
}

inline sKSpecRegs *Thread::getSpecRegs() const {
	return const_cast<sKSpecRegs*>(specRegLevels) + intrptLevel - 1;
}

inline void Thread::pushSpecRegs() {
	Thread *t = Thread::getRunning();
	sKSpecRegs *sregs = t->specRegLevels + t->intrptLevel - 1;
	cpu_getKSpecials(&sregs->rbb,&sregs->rww,&sregs->rxx,&sregs->ryy,&sregs->rzz);
}

inline void Thread::popSpecRegs() {
	Thread *t = Thread::getRunning();
	sKSpecRegs *sregs = t->specRegLevels + t->intrptLevel - 1;
	cpu_setKSpecials(sregs->rbb,sregs->rww,sregs->rxx,sregs->ryy,sregs->rzz);
}

inline uint64_t ThreadBase::getTSC() {
	return cpu_rdtsc();
}

inline uint64_t ThreadBase::ticksPerSec() {
	return cpu_getSpeed();
}
