/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/task/uenv.h>
#include <sys/mem/paging.h>
#include <sys/syscalls/signals.h>
#include <sys/syscalls.h>
#include <errors.h>

int sysc_setSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	fSignal handler = (fSignal)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();

	/* address should be valid */
	if(handler != SIG_IGN && handler != SIG_DFL && !paging_isRangeUserReadable((uintptr_t)handler,1))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if(signal == (tSig)SIG_RET)
		t->proc->sigRetAddr = (uintptr_t)handler;
	else {
		/* no signal-ret-address known yet? */
		if(t->proc->sigRetAddr == 0)
			SYSC_ERROR(stack,ERR_INVALID_ARGS);

		/* check signal */
		if(!sig_canHandle(signal))
			SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

		if(handler == SIG_DFL)
			sig_unsetHandler(t->tid,signal);
		else {
			int res = sig_setHandler(t->tid,signal,handler);
			if(res < 0)
				SYSC_ERROR(stack,res);
		}
	}
	SYSC_RET1(stack,0);
}

int sysc_ackSignal(sIntrptStackFrame *stack) {
	int res;
	sThread *t = thread_getRunning();
	tSig signal = sig_ackHandling(t->tid);
	if((res = uenv_finishSignalHandler(stack,signal)) < 0)
		SYSC_ERROR(stack,res);
	/* we don't set the error-code on the stack here */
	return 0;
}

int sysc_sendSignalTo(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	tSig signal = (tSig)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	/* store tid and check via thread_getById() because if the thread is destroyed, we can't access
	 * it anymore */
	tTid tid = t->tid;

	if(!sig_canSend(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
	if(pid != INVALID_PID && !proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	if(pid != INVALID_PID)
		sig_addSignalFor(pid,signal);
	else
		sig_addSignal(signal);

	/* choose another thread if we've killed ourself */
	if(thread_getById(tid) == NULL)
		thread_switch();

	SYSC_RET1(stack,0);
}
