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

#pragma once

#include <arch/x86/fpu.h>
#include <common.h>
#include <cpu.h>
#include <semaphore.h>

class Thread : public ThreadBase {
	friend class ThreadBase;

	Thread(Proc *p,uint8_t flags) : ThreadBase(p,flags), kernelStack(), fpuState(), fpuSem(0) {
	}

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
	const FPU::XState *getFPUState() const {
		return fpuState;
	}
	FPU::XState **getFPUStatePtr() {
		return &fpuState;
	}
	BaseSem &getFPUSem() {
		return fpuSem;
	}

private:
	static void startup() asm("thread_startup");
	static bool save(ThreadRegs *saveArea) asm("thread_save");
	static bool resume(uintptr_t pageDir,const ThreadRegs *saveArea,SpinLock *lock) asm("thread_resume");

	uintptr_t kernelStack;
	/* FPU-state; initially NULL */
	FPU::XState *fpuState;
	BaseSem fpuSem;
};

inline Thread *ThreadBase::getRunning() {
	ulong sp = CPU::getSP();
	Thread** tptr = (Thread**)((sp & ~(PAGE_SIZE - 1)) + (PAGE_SIZE - sizeof(ulong)));
	return *tptr;
}

inline void ThreadBase::setRunning(Thread *t) {
	ulong sp = CPU::getSP();
	ThreadBase** tptr = (ThreadBase**)((sp & ~(PAGE_SIZE - 1)) + (PAGE_SIZE - sizeof(ulong)));
	*tptr = t;
}

inline IntrptStackFrame *ThreadBase::getUserState() const {
	uintptr_t bottom = static_cast<const Thread*>(this)->getKernelStack();
	return (IntrptStackFrame*)(bottom + PAGE_SIZE - sizeof(ulong) - sizeof(IntrptStackFrame));
}

inline size_t ThreadBase::getThreadFrmCnt() {
	return INITIAL_STACK_PAGES;
}
