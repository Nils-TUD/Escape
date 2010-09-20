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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/sched.h>
#include <sys/machine/timer.h>
#include <sys/mem/kheap.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls.h>
#include <errors.h>

void sysc_gettid(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,t->tid);
}

void sysc_getThreadCount(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,sll_length(t->proc->threads));
}

void sysc_startThread(sIntrptStackFrame *stack) {
	u32 entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	s32 res = proc_startThread(entryPoint,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_exit(sIntrptStackFrame *stack) {
	s32 exitCode = (s32)SYSC_ARG1(stack);
	proc_destroyThread(exitCode);
	thread_switch();
	util_panic("We shouldn't get here...");
}

void sysc_getCycles(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	uLongLong cycles;
	cycles.val64 = t->stats.kcycleCount.val64 + t->stats.ucycleCount.val64;
	SYSC_RET1(stack,cycles.val32.lower);
	SYSC_RET2(stack,cycles.val32.upper);
}

void sysc_sleep(sIntrptStackFrame *stack) {
	u32 msecs = SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	s32 res;
	if((res = timer_sleepFor(t->tid,msecs)) < 0)
		SYSC_ERROR(stack,res);
	thread_switch();
	/* ensure that we're no longer in the timer-list. this may for example happen if we get a signal
	 * and the sleep-time was not over yet. */
	timer_removeThread(t->tid);
	if(sig_hasSignalFor(t->tid))
		SYSC_ERROR(stack,ERR_INTERRUPTED);
	SYSC_RET1(stack,0);
}

void sysc_yield(sIntrptStackFrame *stack) {
	UNUSED(stack);
	thread_switch();
}

void sysc_wait(sIntrptStackFrame *stack) {
	u16 events = (u8)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	if((events & ~EV_USER_WAIT_MASK) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check whether there is a chance that we'll wake up again; if we already have a message
	 * that we should wait for, don't start waiting */
	if(!vfs_msgAvailableFor(t->tid,events)) {
		thread_wait(t->tid,0,events);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	SYSC_RET1(stack,0);
}

void sysc_notify(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	u16 events = (u8)SYSC_ARG2(stack);

	if((events & ~EV_USER_NOTIFY_MASK) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	thread_wakeup(tid,events);
	SYSC_RET1(stack,0);
}

void sysc_join(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	sThread *tt = thread_getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* wait until this thread doesn't exist anymore */
	do {
		thread_wait(t->tid,t->proc,EV_THREAD_DIED);
		thread_switchNoSigs();
	}
	while(thread_getById(tid) != NULL);

	SYSC_RET1(stack,0);
}

void sysc_suspend(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	sThread *tt = thread_getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* suspend it */
	thread_setSuspended(tt->tid,true);
	SYSC_RET1(stack,0);
}

void sysc_resume(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	sThread *tt = thread_getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* resume it */
	thread_setSuspended(tt->tid,false);
	SYSC_RET1(stack,0);
}
