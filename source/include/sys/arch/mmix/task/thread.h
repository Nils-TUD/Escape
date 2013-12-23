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
	KSpecRegs *getSpecRegs() const;

	/**
	 * @return the frame mapped at KERNEL_STACK
	 */
	frameno_t getKernelStack() const {
		return kstackFrame;
	}

private:
	static void startup() asm("thread_startup");
	static int initSave(ThreadRegs *saveArea,void *newStack) asm("thread_initSave");
	static int doSwitchTo(ThreadRegs *oldArea,ThreadRegs *newArea,uint64_t rv,tid_t tid)
		asm("thread_doSwitchTo");

	/* the frame mapped at KERNEL_STACK */
	frameno_t kstackFrame;
	/* use as a temporary kernel-stack for cloning */
	frameno_t tempStack;
	/* when handling a signal, we have to backup these registers */
	KSpecRegs specRegLevels[MAX_INTRPT_LEVELS];
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

inline KSpecRegs *Thread::getSpecRegs() const {
	return const_cast<KSpecRegs*>(specRegLevels) + intrptLevel - 1;
}

inline void Thread::pushSpecRegs() {
	Thread *t = Thread::getRunning();
	KSpecRegs *sregs = t->specRegLevels + t->intrptLevel - 1;
	CPU::getKSpecials(&sregs->rbb,&sregs->rww,&sregs->rxx,&sregs->ryy,&sregs->rzz);
}

inline void Thread::popSpecRegs() {
	Thread *t = Thread::getRunning();
	KSpecRegs *sregs = t->specRegLevels + t->intrptLevel - 1;
	CPU::setKSpecials(sregs->rbb,sregs->rww,sregs->rxx,sregs->ryy,sregs->rzz);
}
