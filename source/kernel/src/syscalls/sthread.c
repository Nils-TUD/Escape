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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/sched.h>
#include <sys/task/event.h>
#include <sys/task/lock.h>
#include <sys/task/timer.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/vfs/vfs.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls.h>
#include <sys/util.h>
#include <string.h>
#include <errors.h>

#define MAX_WAIT_OBJECTS		32

static int sysc_doWait(const sWaitObject *uobjects,size_t objCount,pid_t pid,ulong ident);
static int sysc_doWaitLoop(const sWaitObject *uobjects,size_t objCount,const file_t *objFiles,
		pid_t pid,ulong ident);

int sysc_gettid(sIntrptStackFrame *stack) {
	const sThread *t = thread_getRunning();
	SYSC_RET1(stack,t->tid);
}

int sysc_getThreadCount(sIntrptStackFrame *stack) {
	const sThread *t = thread_getRunning();
	SYSC_RET1(stack,sll_length(t->proc->threads));
}

int sysc_startThread(sIntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	int res = proc_startThread(entryPoint,0,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_exit(sIntrptStackFrame *stack) {
	int exitCode = (int)SYSC_ARG1(stack);
	proc_exit(exitCode);
	thread_switch();
	util_panic("We shouldn't get here...");
	SYSC_RET1(stack,0);
}

int sysc_getCycles(sIntrptStackFrame *stack) {
	const sThread *t = thread_getRunning();
	uLongLong cycles;
	cycles.val64 = t->stats.kcycleCount.val64 + t->stats.ucycleCount.val64;
	SYSC_SETRET2(stack,cycles.val32.upper);
	SYSC_RET1(stack,cycles.val32.lower);
}

int sysc_sleep(sIntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	const sThread *t = thread_getRunning();
	int res;
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

int sysc_yield(sIntrptStackFrame *stack) {
	thread_switch();
	SYSC_RET1(stack,0);
}

int sysc_wait(sIntrptStackFrame *stack) {
	sWaitObject *uobjects = (sWaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)uobjects,objCount * sizeof(sWaitObject)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = sysc_doWait(uobjects,objCount,KERNEL_PID,0);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_waitUnlock(sIntrptStackFrame *stack) {
	sWaitObject *uobjects = (sWaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	uint ident = SYSC_ARG3(stack);
	bool global = (bool)SYSC_ARG4(stack);
	pid_t pid = proc_getRunning();
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)uobjects,objCount * sizeof(sWaitObject)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* wait and release the lock before going to sleep */
	res = sysc_doWait(uobjects,objCount,global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_notify(sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	uint events = SYSC_ARG2(stack);
	sThread *t = thread_getById(tid);

	if((events & ~EV_USER_NOTIFY_MASK) != 0 || t == NULL)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	ev_wakeupThread(t,events);
	SYSC_RET1(stack,0);
}

int sysc_lock(sIntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	ushort flags = (uint)SYSC_ARG3(stack);
	pid_t pid = proc_getRunning();

	int res = lock_aquire(global ? INVALID_PID : pid,ident,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unlock(sIntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	pid_t pid = proc_getRunning();

	int res = lock_release(global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_join(sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	if(tid != 0) {
		const sThread *tt = thread_getById(tid);
		/* just threads from the own process */
		if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	proc_join(tid);
	SYSC_RET1(stack,0);
}

int sysc_suspend(sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	const sThread *t = thread_getRunning();
	sThread *tt = thread_getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	ev_suspend(tt);
	SYSC_RET1(stack,0);
}

int sysc_resume(sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	const sThread *t = thread_getRunning();
	sThread *tt = thread_getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	ev_unsuspend(tt);
	SYSC_RET1(stack,0);
}

static int sysc_doWait(USER const sWaitObject *uobjects,size_t objCount,pid_t pid,ulong ident) {
	file_t objFiles[MAX_WAIT_OBJECTS];
	size_t i;
	int res;
	/* first request the files from the file-descriptors */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* translate fd to node-number */
			objFiles[i] = proc_reqFile((int)uobjects[i].object);
			if(objFiles[i] < 0) {
				for(; i > 0; i--)
					proc_relFile(objFiles[i - 1]);
				return ERR_INVALID_ARGS;
			}
		}
	}

	/* now wait */
	res = sysc_doWaitLoop(uobjects,objCount,objFiles,pid,ident);

	/* release them */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE))
			proc_relFile(objFiles[i]);
	}
	return res;
}

static int sysc_doWaitLoop(USER const sWaitObject *uobjects,size_t objCount,const file_t *objFiles,
		pid_t pid,ulong ident) {
	sWaitObject kobjects[MAX_WAIT_OBJECTS];
	size_t i;

	/* copy to kobjects and check */
	for(i = 0; i < objCount; i++) {
		kobjects[i].events = uobjects[i].events;
		if(kobjects[i].events & ~(EV_USER_WAIT_MASK))
			return ERR_INVALID_ARGS;
		if(kobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* check flags */
			if(kobjects[i].events & EV_CLIENT) {
				if(kobjects[i].events & ~(EV_CLIENT))
					return ERR_INVALID_ARGS;
			}
			else if(kobjects[i].events & ~(EV_RECEIVED_MSG | EV_DATA_READABLE))
				return ERR_INVALID_ARGS;
			kobjects[i].object = objFiles[i];
		}
		else
			kobjects[i].object = uobjects[i].object;
	}
	return vfs_waitFor(kobjects,objCount,true,pid,ident);
}
