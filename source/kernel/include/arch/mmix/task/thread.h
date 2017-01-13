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

#include <common.h>
#include <cpu.h>

#define MAX_INTRPT_LEVELS		3

class Thread : public ThreadBase {
	friend class ThreadBase;

	Thread(Proc *p,uint8_t flags)
		: ThreadBase(p,flags), intrptLevel(), kstackFrame(), tempStack(-1), specRegLevels(),
		  userState() {
	}

public:
	void pushSpecRegs(IntrptStackFrame *state);
	void popSpecRegs();
	KSpecRegs *getSpecRegs() const;

	size_t getIntrptLevel() const {
		return intrptLevel - 1;
	}

	/**
	 * @return the frame mapped at KERNEL_STACK
	 */
	frameno_t getKernelStack() const {
		return kstackFrame;
	}

	void makeIdle() {
		flags |= T_IDLE;
	}

private:
	static void startup() asm("thread_startup");
	static int initSave(ThreadRegs *saveArea,void *newStack) asm("thread_initSave");
	static int doSwitchTo(ThreadRegs *oldArea,ThreadRegs *newArea,uint64_t rv,tid_t tid)
		asm("thread_doSwitchTo");

	size_t intrptLevel;
	/* the frame mapped at KERNEL_STACK */
	frameno_t kstackFrame;
	/* use as a temporary kernel-stack for cloning */
	frameno_t tempStack;
	/* when handling a signal, we have to backup these registers */
	KSpecRegs specRegLevels[MAX_INTRPT_LEVELS];
	IntrptStackFrame *userState[MAX_INTRPT_LEVELS];
	static Thread *cur;
};

inline size_t ThreadBase::getThreadFrmCnt() {
	return INITIAL_STACK_PAGES * 2;
}

inline Thread *ThreadBase::getRunning() {
	return Thread::cur;
}

inline void ThreadBase::setRunning(Thread *t) {
	Thread::cur = t;
}

inline IntrptStackFrame *ThreadBase::getUserState() const {
	const Thread *t = static_cast<const Thread*>(this);
	return t->userState[t->intrptLevel - 1];
}

inline KSpecRegs *Thread::getSpecRegs() const {
	// this occurs during startup in UEnv::setupProc() :(
	if(intrptLevel == 0)
		return const_cast<KSpecRegs*>(specRegLevels);
	return const_cast<KSpecRegs*>(specRegLevels) + intrptLevel - 1;
}

inline void Thread::pushSpecRegs(IntrptStackFrame *state) {
	userState[intrptLevel] = state;
	KSpecRegs *sregs = specRegLevels + intrptLevel;
	CPU::getKSpecials(&sregs->rbb,&sregs->rww,&sregs->rxx,&sregs->ryy,&sregs->rzz);
	intrptLevel++;
}

inline void Thread::popSpecRegs() {
	intrptLevel--;
	KSpecRegs *sregs = specRegLevels + intrptLevel;
	CPU::setKSpecials(sregs->rbb,sregs->rww,sregs->rxx,sregs->ryy,sregs->rzz);
}
