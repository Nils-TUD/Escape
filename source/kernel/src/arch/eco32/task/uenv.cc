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
#include <string.h>
#include <errno.h>
#include <assert.h>

void UEnv::startSignalHandler(Thread *t,IntrptStackFrame *stack,int sig,Signals::handler_func handler) {
	uint32_t *sp = (uint32_t*)stack->r[29];
	if(!PageDir::isInUserSpace((uintptr_t)(sp - REG_COUNT),REG_COUNT * sizeof(uint32_t)))
		goto error;

	/* we have to decrease $30, because when returning from the signal, we'll enter it by a trap,
	 * so that $30 will be increased */
	stack->r[30] -= 4;

	UserAccess::write(sp - REG_COUNT,stack->r,REG_COUNT * sizeof(uint32_t));
	if(t->isFaulted())
		goto error;

	/* signal-number as arguments */
	stack->r[4] = sig;
	/* set new stack-pointer */
	stack->r[29] = (uint32_t)(sp - REG_COUNT);
	/* the process should continue here */
	stack->r[30] = (uint32_t)handler;
	/* and return here after handling the signal */
	stack->r[31] = t->getProc()->getSigRetAddr();
	return;

error:
	Proc::terminate(1,SIG_SEGFAULT);
	A_UNREACHED;
}

int UEnvBase::finishSignalHandler(IntrptStackFrame *stack) {
	uint32_t *sp = (uint32_t*)stack->r[29];
	UserAccess::read(stack->r,sp,REG_COUNT * sizeof(uint32_t));
	return 0;
}

bool UEnvBase::setupProc(int argc,int envc,const char *args,size_t argsSize,
         const ELF::StartupInfo *info,uintptr_t entryPoint,A_UNUSED int fd) {
	Thread *t = Thread::getRunning();
	IntrptStackFrame *frame = t->getIntrptStack();

	ulong *sp = initProcStack(argc,envc,args,argsSize,info->progEntry);
	if(!sp)
		return false;

	/* set entry-point and stack-pointer */
	frame->r[29] = (ulong)sp;
	frame->r[30] = entryPoint - 4; /* we'll skip the trap-instruction for syscalls */
	return true;
}

void *UEnvBase::setupThread(const void *arg,uintptr_t tentryPoint) {
	return initThreadStack(arg,tentryPoint);
}
