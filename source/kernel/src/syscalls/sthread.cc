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
#include <sys/task/fd.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/vfs/vfs.h>
#include <sys/syscalls/thread.h>
#include <sys/syscalls.h>
#include <sys/util.h>
#include <string.h>
#include <errno.h>

#define MAX_WAIT_OBJECTS		32

static int sysc_doWait(const Event::WaitObject *uobjects,size_t objCount,time_t maxWaitTime,
		pid_t pid,ulong ident);
static int sysc_doWaitLoop(const Event::WaitObject *uobjects,size_t objCount,
		sFile **objFiles,time_t maxWaitTime,pid_t pid,ulong ident);

int sysc_gettid(Thread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getTid());
}

int sysc_getthreadcnt(Thread *t,sIntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getThreadCount());
}

int sysc_startthread(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	int res = Proc::startThread(entryPoint,0,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_exit(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	int exitCode = (int)SYSC_ARG1(stack);
	Proc::exit(exitCode);
	Thread::switchAway();
	util_panic("We shouldn't get here...");
	SYSC_RET1(stack,0);
}

int sysc_getcycles(Thread *t,sIntrptStackFrame *stack) {
	uLongLong cycles;
	cycles.val64 = t->getStats().curCycleCount;
	SYSC_SETRET2(stack,cycles.val32.upper);
	SYSC_RET1(stack,cycles.val32.lower);
}

int sysc_alarm(Thread *t,sIntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if((res = timer_sleepFor(t->getTid(),msecs,false)) < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_sleep(Thread *t,sIntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if((res = timer_sleepFor(t->getTid(),msecs,true)) < 0)
		SYSC_ERROR(stack,res);
	Thread::switchAway();
	/* ensure that we're no longer in the timer-list. this may for example happen if we get a signal
	 * and the sleep-time was not over yet. */
	timer_removeThread(t->getTid());
	if(sig_hasSignalFor(t->getTid()))
		SYSC_ERROR(stack,-EINTR);
	SYSC_RET1(stack,0);
}

int sysc_yield(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	Thread::switchAway();
	SYSC_RET1(stack,0);
}

int sysc_wait(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	const Event::WaitObject *uobjects = (const Event::WaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	time_t maxWaitTime = SYSC_ARG3(stack);
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)uobjects,objCount * sizeof(Event::WaitObject)))
		SYSC_ERROR(stack,-EFAULT);

	res = sysc_doWait(uobjects,objCount,maxWaitTime,KERNEL_PID,0);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_waitunlock(Thread *t,sIntrptStackFrame *stack) {
	const Event::WaitObject *uobjects = (const Event::WaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	uint ident = SYSC_ARG3(stack);
	bool global = (bool)SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)uobjects,objCount * sizeof(Event::WaitObject)))
		SYSC_ERROR(stack,-EFAULT);

	/* wait and release the lock before going to sleep */
	res = sysc_doWait(uobjects,objCount,0,global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_notify(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	uint events = SYSC_ARG2(stack);
	Thread *nt = Thread::getById(tid);

	if((events & ~EV_USER_NOTIFY_MASK) != 0 || nt == NULL)
		SYSC_ERROR(stack,-EINVAL);
	Event::wakeupThread(nt,events);
	SYSC_RET1(stack,0);
}

int sysc_lock(Thread *t,sIntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	ushort flags = (uint)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::aquire(global ? INVALID_PID : pid,ident,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unlock(Thread *t,sIntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::release(global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_join(Thread *t,sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	if(tid != 0) {
		const Thread *tt = Thread::getById(tid);
		/* just threads from the own process */
		if(tt == NULL || tt->getTid() == t->getTid() || tt->getProc()->getPid() != t->getProc()->getPid())
			SYSC_ERROR(stack,-EINVAL);
	}

	Proc::join(tid);
	SYSC_RET1(stack,0);
}

int sysc_suspend(Thread *t,sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->getTid() == t->getTid() || tt->getProc()->getPid() != t->getProc()->getPid())
		SYSC_ERROR(stack,-EINVAL);
	Event::suspend(tt);
	SYSC_RET1(stack,0);
}

int sysc_resume(Thread *t,sIntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->getTid() == t->getTid() || tt->getProc()->getPid() != t->getProc()->getPid())
		SYSC_ERROR(stack,-EINVAL);
	Event::unsuspend(tt);
	SYSC_RET1(stack,0);
}

static int sysc_doWait(USER const Event::WaitObject *uobjects,size_t objCount,
		time_t maxWaitTime,pid_t pid,ulong ident) {
	sFile *objFiles[MAX_WAIT_OBJECTS];
	size_t i;
	int res;
	/* first request the files from the file-descriptors */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* translate fd to node-number */
			objFiles[i] = fd_request((int)uobjects[i].object);
			if(objFiles[i] == NULL) {
				for(; i > 0; i--)
					fd_release(objFiles[i - 1]);
				return -EBADF;
			}
		}
	}

	/* now wait */
	res = sysc_doWaitLoop(uobjects,objCount,objFiles,maxWaitTime,pid,ident);

	/* release them */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE))
			fd_release(objFiles[i]);
	}
	return res;
}

static int sysc_doWaitLoop(USER const Event::WaitObject *uobjects,size_t objCount,
		sFile **objFiles,time_t maxWaitTime,pid_t pid,ulong ident) {
	Event::WaitObject kobjects[MAX_WAIT_OBJECTS];
	size_t i;

	/* copy to kobjects and check */
	for(i = 0; i < objCount; i++) {
		kobjects[i].events = uobjects[i].events;
		if(kobjects[i].events & ~(EV_USER_WAIT_MASK))
			return -EINVAL;
		if(kobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* check flags */
			if(kobjects[i].events & EV_CLIENT) {
				if(kobjects[i].events & ~(EV_CLIENT))
					return -EINVAL;
			}
			else if(kobjects[i].events & ~(EV_RECEIVED_MSG | EV_DATA_READABLE))
				return -EINVAL;
			kobjects[i].object = (evobj_t)objFiles[i];
		}
		else
			kobjects[i].object = uobjects[i].object;
	}
	return vfs_waitFor(kobjects,objCount,maxWaitTime,true,pid,ident);
}
