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
#include <sys/task/filedesc.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/vfs/vfs.h>
#include <sys/syscalls.h>
#include <sys/util.h>
#include <string.h>
#include <errno.h>

#define MAX_WAIT_OBJECTS		32

static int doWait(const Event::WaitObject *uobjects,size_t objCount,time_t maxWaitTime,
		pid_t pid,ulong ident);
static int doWaitLoop(const Event::WaitObject *uobjects,size_t objCount,
		OpenFile **objFiles,time_t maxWaitTime,pid_t pid,ulong ident);

int Syscalls::gettid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getTid());
}

int Syscalls::getthreadcnt(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getThreadCount());
}

int Syscalls::startthread(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	int res = Proc::startThread(entryPoint,0,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::exit(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int exitCode = (int)SYSC_ARG1(stack);
	Proc::exit(exitCode);
	Thread::switchAway();
	Util::panic("We shouldn't get here...");
	SYSC_RET1(stack,0);
}

int Syscalls::getcycles(Thread *t,IntrptStackFrame *stack) {
	uLongLong cycles;
	cycles.val64 = t->getStats().curCycleCount;
	SYSC_SETRET2(stack,cycles.val32.upper);
	SYSC_RET1(stack,cycles.val32.lower);
}

int Syscalls::alarm(Thread *t,IntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if((res = Timer::sleepFor(t->getTid(),msecs,false)) < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::sleep(Thread *t,IntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if((res = Timer::sleepFor(t->getTid(),msecs,true)) < 0)
		SYSC_ERROR(stack,res);
	Thread::switchAway();
	/* ensure that we're no longer in the timer-list. this may for example happen if we get a signal
	 * and the sleep-time was not over yet. */
	Timer::removeThread(t->getTid());
	if(Signals::hasSignalFor(t->getTid()))
		SYSC_ERROR(stack,-EINTR);
	SYSC_RET1(stack,0);
}

int Syscalls::yield(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	Thread::switchAway();
	SYSC_RET1(stack,0);
}

int Syscalls::wait(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	const Event::WaitObject *uobjects = (const Event::WaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	time_t maxWaitTime = SYSC_ARG3(stack);
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,-EINVAL);
	if(!PageDir::isInUserSpace((uintptr_t)uobjects,objCount * sizeof(Event::WaitObject)))
		SYSC_ERROR(stack,-EFAULT);

	res = doWait(uobjects,objCount,maxWaitTime,KERNEL_PID,0);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::waitunlock(Thread *t,IntrptStackFrame *stack) {
	const Event::WaitObject *uobjects = (const Event::WaitObject*)SYSC_ARG1(stack);
	size_t objCount = SYSC_ARG2(stack);
	uint ident = SYSC_ARG3(stack);
	bool global = (bool)SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	int res;

	if(objCount == 0 || objCount > MAX_WAIT_OBJECTS)
		SYSC_ERROR(stack,-EINVAL);
	if(!PageDir::isInUserSpace((uintptr_t)uobjects,objCount * sizeof(Event::WaitObject)))
		SYSC_ERROR(stack,-EFAULT);

	/* wait and release the lock before going to sleep */
	res = doWait(uobjects,objCount,0,global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::notify(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	uint events = SYSC_ARG2(stack);
	Thread *nt = Thread::getById(tid);

	if((events & ~EV_USER_NOTIFY_MASK) != 0 || nt == NULL)
		SYSC_ERROR(stack,-EINVAL);
	Event::wakeupThread(nt,events);
	SYSC_RET1(stack,0);
}

int Syscalls::lock(Thread *t,IntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	ushort flags = (uint)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::acquire(global ? INVALID_PID : pid,ident,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::unlock(Thread *t,IntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::release(global ? INVALID_PID : pid,ident);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::join(Thread *t,IntrptStackFrame *stack) {
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

int Syscalls::suspend(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->getTid() == t->getTid() || tt->getProc()->getPid() != t->getProc()->getPid())
		SYSC_ERROR(stack,-EINVAL);
	Event::suspend(tt);
	SYSC_RET1(stack,0);
}

int Syscalls::resume(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(tt == NULL || tt->getTid() == t->getTid() || tt->getProc()->getPid() != t->getProc()->getPid())
		SYSC_ERROR(stack,-EINVAL);
	Event::unsuspend(tt);
	SYSC_RET1(stack,0);
}

static int doWait(USER const Event::WaitObject *uobjects,size_t objCount,
		time_t maxWaitTime,pid_t pid,ulong ident) {
	OpenFile *objFiles[MAX_WAIT_OBJECTS];
	size_t i;
	int res;
	/* first request the files from the file-descriptors */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			/* translate fd to node-number */
			objFiles[i] = FileDesc::request((int)uobjects[i].object);
			if(objFiles[i] == NULL) {
				for(; i > 0; i--)
					FileDesc::release(objFiles[i - 1]);
				return -EBADF;
			}
		}
	}

	/* now wait */
	res = doWaitLoop(uobjects,objCount,objFiles,maxWaitTime,pid,ident);

	/* release them */
	for(i = 0; i < objCount; i++) {
		if(uobjects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE))
			FileDesc::release(objFiles[i]);
	}
	return res;
}

static int doWaitLoop(USER const Event::WaitObject *uobjects,size_t objCount,
		OpenFile **objFiles,time_t maxWaitTime,pid_t pid,ulong ident) {
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
