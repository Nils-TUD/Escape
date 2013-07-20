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

#include <sys/common.h>
#include <sys/task/signals.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/uenv.h>
#include <sys/mem/paging.h>
#include <sys/syscalls/signals.h>
#include <sys/syscalls.h>
#include <errno.h>

int sysc_signal(Thread *t,sIntrptStackFrame *stack) {
	int signal = (int)SYSC_ARG1(stack);
	fSignal handler = (fSignal)SYSC_ARG2(stack);
	fSignal old = SIG_ERR;

	/* address should be valid */
	if(handler != SIG_IGN && handler != SIG_DFL && !paging_isInUserSpace((uintptr_t)handler,1))
		SYSC_ERROR(stack,(long)SIG_ERR);

	if(signal == (int)SIG_RET)
		t->getProc()->setSigRetAddr((uintptr_t)handler);
	else {
		/* no signal-ret-address known yet? */
		if(t->getProc()->getSigRetAddr() == 0)
			SYSC_ERROR(stack,(long)SIG_ERR);

		/* check signal */
		if(!sig_canHandle(signal))
			SYSC_ERROR(stack,(long)SIG_ERR);

		if(handler == SIG_DFL)
			old = sig_unsetHandler(t->getTid(),signal);
		else {
			if(sig_setHandler(t->getTid(),signal,handler,&old) < 0)
				SYSC_ERROR(stack,(long)SIG_ERR);
		}
	}
	SYSC_RET1(stack,(long)old);
}

int sysc_acksignal(Thread *t,sIntrptStackFrame *stack) {
	int res;
	int signal = sig_ackHandling(t->getTid());
	if((res = uenv_finishSignalHandler(stack,signal)) < 0)
		SYSC_ERROR(stack,res);
	/* we don't set the error-code on the stack here */
	return 0;
}

int sysc_kill(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	pid_t pid = (pid_t)SYSC_ARG1(stack);
	int signal = (int)SYSC_ARG2(stack);

	if(!sig_canSend(signal))
		SYSC_ERROR(stack,-EINVAL);

	if(pid != INVALID_PID)
		Proc::addSignalFor(pid,signal);
	else
		sig_addSignal(signal);
	SYSC_RET1(stack,0);
}
