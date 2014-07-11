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
#include <sys/task/uenv.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/useraccess.h>
#include <sys/spinlock.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if defined(__x86_64__)
// take care of red-zone
#	define REG_COUNT		(128 / sizeof(ulong) + 17)
#else
#	define REG_COUNT		9
#endif

void UEnv::startSignalHandler(Thread *t,IntrptStackFrame *stack,int sig,Signals::handler_func handler) {
	ulong *sp = (ulong*)stack->getSP();
	if(!PageDir::isInUserSpace((uintptr_t)(sp - REG_COUNT),REG_COUNT * sizeof(ulong)))
		goto error;

#if defined(__x86_64__)
	sp -= 128 / sizeof(ulong);
#endif

	/* the ret-instruction of sigRet() should go to the old eip */
	UserAccess::writeVar(--sp,stack->getIP());
	/* save regs */
	UserAccess::writeVar(--sp,stack->getFlags());
	UserAccess::writeVar(--sp,stack->REG(ax));
	UserAccess::writeVar(--sp,stack->REG(bx));
	UserAccess::writeVar(--sp,stack->REG(cx));
	UserAccess::writeVar(--sp,stack->REG(dx));
	UserAccess::writeVar(--sp,stack->REG(di));
	UserAccess::writeVar(--sp,stack->REG(si));
#if defined(__x86_64__)
	UserAccess::writeVar(--sp,stack->r8);
	UserAccess::writeVar(--sp,stack->r9);
	UserAccess::writeVar(--sp,stack->r10);
	UserAccess::writeVar(--sp,stack->r11);
	UserAccess::writeVar(--sp,stack->r12);
	UserAccess::writeVar(--sp,stack->r13);
	UserAccess::writeVar(--sp,stack->r14);
	UserAccess::writeVar(--sp,stack->r15);
#endif
	/* sigRet will remove the argument, restore the register,
	 * acknoledge the signal and return to eip */
	UserAccess::writeVar(--sp,(ulong)t->getProc()->getSigRetAddr());

	/* if one of theses accesses failed, terminate */
	if(t->isFaulted())
		goto error;

	stack->setIP((uintptr_t)handler);
	stack->setSP((ulong)sp);
	/* signal-number as argument */
	stack->ARG_1 = sig;
	return;

error:
	Proc::terminate(1,SIGSEGV);
	A_UNREACHED;
}

int UEnvBase::finishSignalHandler(IntrptStackFrame *stack) {
	ulong *sp = (ulong*)stack->getSP();
	if(!PageDir::isInUserSpace((uintptr_t)sp,REG_COUNT * sizeof(ulong)))
		goto error;

	/* restore regs */
#if defined(__x86_64__)
	UserAccess::readVar(&stack->r15,sp++);
	UserAccess::readVar(&stack->r14,sp++);
	UserAccess::readVar(&stack->r13,sp++);
	UserAccess::readVar(&stack->r12,sp++);
	UserAccess::readVar(&stack->r11,sp++);
	UserAccess::readVar(&stack->r10,sp++);
	UserAccess::readVar(&stack->r9,sp++);
	UserAccess::readVar(&stack->r8,sp++);
#endif
	UserAccess::readVar(&stack->REG(si),sp++);
	UserAccess::readVar(&stack->REG(di),sp++);
	UserAccess::readVar(&stack->REG(dx),sp++);
	UserAccess::readVar(&stack->REG(cx),sp++);
	UserAccess::readVar(&stack->REG(bx),sp++);
	UserAccess::readVar(&stack->REG(ax),sp++);

	ulong tmp;
	UserAccess::readVar(&tmp,sp++);
	stack->setFlags(tmp);

	/* return */
	UserAccess::readVar(&tmp,sp++);
	stack->setIP(tmp);

#if defined(__x86_64__)
	sp += 128 / sizeof(ulong);
#endif

	stack->setSP((uintptr_t)sp);
	return 0;

error:
	Proc::terminate(1,SIGSEGV);
	A_UNREACHED;
}

bool UEnvBase::setupProc(int argc,int envc,const char *args,size_t argsSize,
		const ELF::StartupInfo *info,uintptr_t entryPoint,int fd) {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *frame = t->getIntrptStack();

	ulong *sp = initProcStack(argc,envc,args,argsSize);
	if(!sp)
		return false;

	/* if its the dynamic linker, give him the filedescriptor, so that he can load it including
	 * all shared libraries */
	if(info->linkerEntry != info->progEntry) {
		assert(fd != -1);
		UserAccess::writeVar(--sp,(ulong)fd);
	}

	frame->setSP((ulong)sp);
	frame->REG(bp) = frame->getSP();
	frame->setIP(entryPoint);
	return true;
}

void *UEnvBase::setupThread(const void *arg,uintptr_t tentryPoint) {
	return initThreadStack(arg,tentryPoint);
}
