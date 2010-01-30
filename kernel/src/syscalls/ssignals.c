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

#include <common.h>
#include <task/signals.h>
#include <task/thread.h>
#include <mem/paging.h>
#include <syscalls/signals.h>
#include <syscalls.h>
#include <errors.h>

void sysc_setSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	fSigHandler handler = (fSigHandler)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* address should be valid */
	if(!paging_isRangeUserReadable((u32)handler,1))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check signal */
	if(!sig_canHandle(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

	err = sig_setHandler(t->tid,signal,handler);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

void sysc_unsetSigHandler(sIntrptStackFrame *stack) {
	tSig signal = (tSig)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if(!sig_canHandle(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);

	sig_unsetHandler(t->tid,signal);

	SYSC_RET1(stack,0);
}

void sysc_ackSignal(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	sig_ackHandling(t->tid);
	SYSC_RET1(stack,0);
}

void sysc_sendSignalTo(sIntrptStackFrame *stack) {
	tPid pid = (tPid)SYSC_ARG1(stack);
	tSig signal = (tSig)SYSC_ARG2(stack);
	u32 data = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();

	if(!sig_canSend(signal))
		SYSC_ERROR(stack,ERR_INVALID_SIGNAL);
	if(pid != INVALID_PID && !proc_exists(pid))
		SYSC_ERROR(stack,ERR_INVALID_PID);

	if(pid != INVALID_PID)
		sig_addSignalFor(pid,signal,data);
	else
		sig_addSignal(signal,data);

	/* choose another thread if we've killed ourself */
	if(t->state != ST_RUNNING)
		thread_switch();

	SYSC_RET1(stack,0);
}
