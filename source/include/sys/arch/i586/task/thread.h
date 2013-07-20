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
#include <sys/task/timer.h>
#include <sys/cpu.h>

class Thread : public ThreadBase {
	friend class ThreadBase;
public:
	/**
	 * Performs the initial switch for APs, because these don't run a thread yet.
	 */
	static void initialSwitch();

	/**
	 * @return lower end of the kernel-stack
	 */
	uintptr_t getKernelStack() const {
		return kernelStack;
	}
	sFPUState **getFPUState() {
		return &fpuState;
	}

private:
	uintptr_t kernelStack;
	/* FPU-state; initially NULL */
	sFPUState *fpuState;
};

inline Thread *ThreadBase::getRunning() {
	uint32_t esp;
	__asm__ (
		"mov	%%esp,%0;"
		/* outputs */
		: "=r" (esp)
		/* inputs */
		:
	);
	Thread** tptr = (Thread**)((esp & 0xFFFFF000) + 0xFFC);
	return *tptr;
}

inline void ThreadBase::setRunning(Thread *t) {
	uint32_t esp;
	__asm__ (
		"mov	%%esp,%0;"
		/* outputs */
		: "=r" (esp)
		/* inputs */
		:
	);
	ThreadBase** tptr = (ThreadBase**)((esp & 0xFFFFF000) + 0xFFC);
	*tptr = t;
}

inline size_t ThreadBase::getThreadFrmCnt(void) {
	return INITIAL_STACK_PAGES;
}

inline uint64_t ThreadBase::getTSC(void) {
	return cpu_rdtsc();
}

inline uint64_t ThreadBase::ticksPerSec(void) {
	return cpu_getSpeed();
}

inline uint64_t ThreadBase::getRuntime() const {
	if(state == Thread::RUNNING) {
		/* if the thread is running, we must take the time since the last scheduling of that thread
		 * into account. this is especially a problem with idle-threads */
		uint64_t cycles = cpu_rdtsc();
		return (stats.runtime + timer_cyclesToTime(cycles - stats.cycleStart));
	}
	return stats.runtime;
}
