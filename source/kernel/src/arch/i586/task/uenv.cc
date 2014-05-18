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
#include <sys/arch/i586/gdt.h>
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

void UEnv::startSignalHandler(Thread *t,IntrptStackFrame *stack,int sig,Signals::handler_func handler) {
	ulong *esp = (ulong*)stack->getSP();
	if(!PageDir::isInUserSpace((uintptr_t)(esp - 9),9 * sizeof(ulong)))
		goto error;

	/* the ret-instruction of sigRet() should go to the old eip */
	UserAccess::writeVar(--esp,stack->getIP());
	/* save regs */
	UserAccess::writeVar(--esp,stack->getFlags());
	UserAccess::writeVar(--esp,stack->eax);
	UserAccess::writeVar(--esp,stack->ebx);
	UserAccess::writeVar(--esp,stack->ecx);
	UserAccess::writeVar(--esp,stack->edx);
	UserAccess::writeVar(--esp,stack->edi);
	UserAccess::writeVar(--esp,stack->esi);
	/* sigRet will remove the argument, restore the register,
	 * acknoledge the signal and return to eip */
	UserAccess::writeVar(--esp,(ulong)t->getProc()->getSigRetAddr());

	/* if one of theses accesses failed, terminate */
	if(t->isFaulted())
		goto error;

	stack->setIP((uintptr_t)handler);
	stack->setSP((ulong)esp);
	/* signal-number as argument */
	stack->eax = sig;
	return;

error:
	Proc::terminate(1,SIG_SEGFAULT);
	A_UNREACHED;
}

int UEnvBase::finishSignalHandler(IntrptStackFrame *stack) {
	ulong *esp = (ulong*)stack->getSP();
	if(!PageDir::isInUserSpace((uintptr_t)esp,9 * sizeof(ulong)))
		goto error;

	/* restore regs */
	UserAccess::readVar(&stack->esi,esp++);
	UserAccess::readVar(&stack->edi,esp++);
	UserAccess::readVar(&stack->edx,esp++);
	UserAccess::readVar(&stack->ecx,esp++);
	UserAccess::readVar(&stack->ebx,esp++);
	UserAccess::readVar(&stack->eax,esp++);

	ulong tmp;
	UserAccess::readVar(&tmp,esp++);
	stack->setFlags(tmp);

	/* return */
	UserAccess::readVar(&tmp,esp++);
	stack->setIP(tmp);

	stack->setSP((uintptr_t)esp);
	return 0;

error:
	Proc::terminate(1,SIG_SEGFAULT);
	A_UNREACHED;
}

bool UEnvBase::setupProc(int argc,int envc,const char *args,size_t argsSize,
		const ELF::StartupInfo *info,uintptr_t entryPoint,int fd) {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *frame = t->getIntrptStack();

	ulong *esp = initProcStack(argc,envc,args,argsSize,info->progEntry);
	if(!esp)
		return false;

	/* if its the dynamic linker, give him the filedescriptor, so that he can load it including
	 * all shared libraries */
	if(info->linkerEntry != info->progEntry) {
		assert(fd != -1);
		UserAccess::writeVar(--esp,(ulong)fd);
	}

	frame->setSP((ulong)esp);
	frame->ebp = frame->getSP();
	frame->setIP(entryPoint);
	return true;
}

void *UEnvBase::setupThread(const void *arg,uintptr_t tentryPoint) {
	return initThreadStack(arg,tentryPoint);
}
