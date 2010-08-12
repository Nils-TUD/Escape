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
#include <sys/mem/paging.h>
#include <sys/syscalls/signals.h>
#include <sys/syscalls.h>
#include <errors.h>

void sysc_setSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	fSigHandler handler = (fSigHandler)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 err = 0;

	/* address should be valid */
	if(handler != SIG_IGN && handler != SIG_DFL && !paging_isRangeUserReadable((u32)handler,1))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	if((s8)signal == SIG_RET)
		t->proc->sigRetAddr = (u32)handler;
	else {
		/* no signal-ret-address known yet? */
		if(t->proc->sigRetAddr == 0)
			SYSC_ERROR(stack,ERR_INVALID_ARGS);

		/* check signal */
		if(!sig_canHandle(signal))
			SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

		if(handler == SIG_DFL)
			sig_unsetHandler(t->tid,signal);
		else
			err = sig_setHandler(t->tid,signal,handler);
		if(err < 0)
			SYSC_ERROR(stack,err);
	}
	SYSC_RET1(stack,err);
}

void sysc_ackSignal(sIntrptStackFrame *stack) {
	u32 *esp;
	sThread *t = thread_getRunning();
	sig_ackHandling(t->tid);

	esp = (u32*)stack->uesp;
	if(!paging_isRangeUserReadable((u32)esp,sizeof(u32) * 9))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* remove args */
	esp += 2;
	/* restore regs */
	stack->esi = *esp++;
	stack->edi = *esp++;
	stack->edx = *esp++;
	stack->ecx = *esp++;
	stack->ebx = *esp++;
	stack->eax = *esp++;
	stack->eflags = *esp++;
	/* return */
	stack->eip = *esp++;
	stack->uesp = (u32)esp;
}

void sysc_sendSignalTo(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	tSig signal = (tSig)SYSC_ARG2(stack);
	u32 data = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	/* store tid and check via thread_getById() because if the thread is destroyed, we can't access
	 * it anymore */
	tTid tid = t->tid;

	if(!sig_canSend(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
	if(pid != INVALID_PID && !proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	if(pid != INVALID_PID)
		sig_addSignalFor(pid,signal,data);
	else
		sig_addSignal(signal,data);

	/* choose another thread if we've killed ourself */
	if(thread_getById(tid) == NULL)
		thread_switch();

	SYSC_RET1(stack,0);
}
