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
#include <task/proc.h>
#include <task/signals.h>
#include <task/thread.h>
#include <task/uenv.h>
#include <task/filedesc.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <syscalls.h>

int Syscalls::signal(Thread *t,IntrptStackFrame *stack) {
	int signal = (int)SYSC_ARG1(stack);
	Signals::handler_func handler = (Signals::handler_func)SYSC_ARG2(stack);
	Signals::handler_func old = SIG_ERR;

	/* address should be valid */
	if(EXPECT_FALSE(handler != SIG_IGN && handler != SIG_DFL &&
			!PageDir::isInUserSpace((uintptr_t)handler,1)))
		SYSC_ERROR(stack,(long)SIG_ERR);

	if(EXPECT_FALSE(signal == (int)SIGRET)) {
		old = SIG_IGN;
		t->getProc()->setSigRetAddr((uintptr_t)handler);
	}
	else {
		/* no signal-ret-address known yet? */
		if(EXPECT_FALSE(t->getProc()->getSigRetAddr() == 0))
			SYSC_ERROR(stack,(long)SIG_ERR);

		/* check signal */
		if(EXPECT_FALSE(!Signals::canHandle(signal)))
			SYSC_ERROR(stack,(long)SIG_ERR);

		old = Signals::setHandler(t,signal,handler);
	}
	if(old == SIG_ERR)
		SYSC_ERROR(stack,(long)old);
	SYSC_SUCCESS(stack,(long)old);
}

int Syscalls::acksignal(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int res;
	if(EXPECT_FALSE((res = UEnv::finishSignalHandler(stack)) < 0))
		SYSC_ERROR(stack,res);
	/* we don't set the error-code on the stack here */
	return 0;
}

int Syscalls::kill(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int signal = (int)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!Signals::canSend(signal)))
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	ScopedFile file(p,fd);
	if(EXPECT_FALSE(!file))
		SYSC_ERROR(stack,-EBADF);

	/* it needs to be a process node */
	if(file->getDev() != VFS_DEV_NO || !IS_PROC(file->getNode()->getMode()) || !file->getNode()->isAlive())
		SYSC_ERROR(stack,-EINVAL);

	/* write access is required */
	if(!(file->getFlags() & VFS_WRITE))
		SYSC_ERROR(stack,-EACCES);

	/* send signal to process */
	int res = Proc::addSignalFor(atoi(file->getNode()->getName()),signal);
	SYSC_RESULT(stack,res);
}
