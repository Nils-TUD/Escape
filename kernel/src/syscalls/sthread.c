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
#include <sys/task/event.h>
#include <sys/task/lock.h>
#include <sys/task/timer.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls.h>
#include <string.h>
#include <errors.h>

static int sysc_doWait(sWaitObject *uobjects,size_t objCount);

void sysc_gettid(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,t->tid);
}

void sysc_getThreadCount(sIntrptStackFrame *stack) {
	sThread *t = thread_getRunning();
	SYSC_RET1(stack,sll_length(t->proc->threads));
}

void sysc_startThread(sIntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	int res = proc_startThread(entryPoint,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_exit(sIntrptStackFrame *stack) {
	int exitCode = (int)SYSC_ARG1(stack);
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
	tTime msecs = SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
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

void sysc_yield(sIntrptStackFrame *stack) {
	UNUSED(stack);
	thread_switch();
}

void sysc_wait(sIntrptStackFrame *stack) {
	sWaitObject *uobjects = (sWaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	int res;

	if(objCount == 0 ||
			!paging_isRangeUserReadable((uintptr_t)uobjects,objCount * sizeof(sWaitObject)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = sysc_doWait(uobjects,objCount);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_waitUnlock(sIntrptStackFrame *stack) {
	sWaitObject *uobjects = (sWaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	uint ident = SYSC_ARG3(stack);
	bool global = (bool)SYSC_ARG4(stack);
	sProc *p = proc_getRunning();
	int res;

	if(objCount == 0 ||
			!paging_isRangeUserReadable((uintptr_t)uobjects,objCount * sizeof(sWaitObject)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* release the lock */
	res = lock_release(global ? INVALID_PID : p->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* now wait */
	res = sysc_doWait(uobjects,objCount);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_notify(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	uint events = SYSC_ARG2(stack);

	if((events & ~EV_USER_NOTIFY_MASK) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	ev_wakeupThread(tid,events);
	SYSC_RET1(stack,0);
}

void sysc_lock(sIntrptStackFrame *stack) {
	uint ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	ushort flags = (uint)SYSC_ARG3(stack);
	sProc *p = proc_getRunning();

	int res = lock_aquire(global ? INVALID_PID : p->pid,ident,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unlock(sIntrptStackFrame *stack) {
	uint ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();

	int res = lock_release(global ? INVALID_PID : p->pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_join(sIntrptStackFrame *stack) {
	tTid tid = (tTid)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	if(tid != 0) {
		sThread *tt = thread_getById(tid);
		/* just threads from the own process */
		if(tt == NULL || tt->tid == t->tid || tt->proc->pid != t->proc->pid)
			SYSC_ERROR(stack,ERR_INVALID_ARGS);
	}

	/* wait until this thread doesn't exist anymore or there are no other threads than ourself */
	do {
		ev_wait(t->tid,EVI_THREAD_DIED,(tEvObj)t->proc);
		thread_switchNoSigs();
	}
	while((tid == 0 && sll_length(t->proc->threads) > 1) ||
		(tid != 0 && thread_getById(tid) != NULL));

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

static int sysc_doWait(sWaitObject *uobjects,size_t objCount) {
	sThread *t = thread_getRunning();
	int res = ERR_INVALID_ARGS;
	size_t i;
	sWaitObject *kobjects = (sWaitObject*)kheap_alloc(objCount * sizeof(sWaitObject));
	if(kobjects == NULL)
		return ERR_NOT_ENOUGH_MEM;
	memcpy(kobjects,uobjects,objCount * sizeof(sWaitObject));

	/* copy to kobjects and check */
	for(i = 0; i < objCount; i++) {
		if(kobjects[i].events & ~(EV_USER_WAIT_MASK))
			goto error;
		if(kobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* translate fd to node-number */
			tFileNo file = proc_fdToFile((tFD)kobjects[i].object);
			if(file < 0)
				goto error;
			/* check flags */
			if(kobjects[i].events & EV_CLIENT) {
				if(kobjects[i].events & ~(EV_CLIENT))
					goto error;
			}
			else if(kobjects[i].events & ~(EV_RECEIVED_MSG | EV_DATA_READABLE))
				goto error;
			kobjects[i].object = (tEvObj)vfs_getVNode(file);
			if(!kobjects[i].object)
				goto error;
		}
	}

	while(true) {
		/* check wether we can wait */
		for(i = 0; i < objCount; i++) {
			if(kobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
				tFileNo file = proc_fdToFile((tFD)uobjects[i].object);
				if((kobjects[i].events & EV_CLIENT) && vfs_getClient(t->proc->pid,&file,1,NULL) >= 0)
					goto done;
				else if((kobjects[i].events & EV_RECEIVED_MSG) && vfs_hasMsg(t->proc->pid,file))
					goto done;
				else if((kobjects[i].events & EV_DATA_READABLE) && vfs_hasData(t->proc->pid,file))
					goto done;
			}
		}

		/* wait */
		if(!ev_waitObjects(t->tid,kobjects,objCount)) {
			res = ERR_NOT_ENOUGH_MEM;
			goto error;
		}
		thread_switch();
		if(sig_hasSignalFor(t->tid)) {
			res = ERR_INTERRUPTED;
			goto error;
		}
		/* if we're waiting for other events, too, we have to wake up */
		for(i = 0; i < objCount; i++) {
			if((kobjects[i].events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)))
				goto done;
		}
	}

done:
	kheap_free(kobjects);
	return 0;

error:
	kheap_free(kobjects);
	return res;
}
