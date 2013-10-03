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
#include <sys/task/lock.h>
#include <sys/task/timer.h>
#include <sys/task/filedesc.h>
#include <sys/mem/cache.h>
#include <sys/mem/pagedir.h>
#include <sys/vfs/vfs.h>
#include <sys/syscalls.h>
#include <sys/util.h>
#include <string.h>
#include <errno.h>

#define MAX_WAIT_OBJECTS		32

static int doWait(uint event,evobj_t object,time_t maxWaitTime,pid_t pid,ulong ident);

int Syscalls::gettid(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getTid());
}

int Syscalls::getthreadcnt(Thread *t,IntrptStackFrame *stack) {
	SYSC_RET1(stack,t->getProc()->getThreadCount());
}

int Syscalls::startthread(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uintptr_t entryPoint = SYSC_ARG1(stack);
	void *arg = (void*)SYSC_ARG2(stack);
	if(EXPECT_FALSE(!PageDir::isInUserSpace(entryPoint,1)))
		SYSC_ERROR(stack, -EINVAL);

	int res = Proc::startThread(entryPoint,0,arg);
	if(EXPECT_FALSE(res < 0))
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
	uint64_t *res = (uint64_t*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)res,sizeof(uint64_t))))
		SYSC_ERROR(stack,-EFAULT);
	*res = t->getStats().curCycleCount;
	SYSC_RET1(stack,0);
}

int Syscalls::alarm(Thread *t,IntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if(EXPECT_FALSE((res = Timer::sleepFor(t->getTid(),msecs,false)) < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::sleep(Thread *t,IntrptStackFrame *stack) {
	time_t msecs = SYSC_ARG1(stack);
	int res;
	if(EXPECT_FALSE((res = Timer::sleepFor(t->getTid(),msecs,true)) < 0))
		SYSC_ERROR(stack,res);
	Thread::switchAway();
	/* ensure that we're no longer in the timer-list. this may for example happen if we get a signal
	 * and the sleep-time was not over yet. */
	Timer::removeThread(t->getTid());
	if(EXPECT_FALSE(t->hasSignalQuick()))
		SYSC_ERROR(stack,-EINTR);
	SYSC_RET1(stack,0);
}

int Syscalls::yield(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	Thread::switchAway();
	SYSC_RET1(stack,0);
}

int Syscalls::wait(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	uint event = (uint)SYSC_ARG1(stack);
	evobj_t object = (evobj_t)SYSC_ARG2(stack);
	time_t maxWaitTime = SYSC_ARG3(stack);

	int res = doWait(event,object,maxWaitTime,KERNEL_PID,0);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::waitunlock(Thread *t,IntrptStackFrame *stack) {
	uint event = (uint)SYSC_ARG1(stack);
	evobj_t object = (evobj_t)SYSC_ARG2(stack);
	uint ident = SYSC_ARG3(stack);
	bool global = (bool)SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();

	/* wait and release the lock before going to sleep */
	int res = doWait(event,object,0,global ? INVALID_PID : pid,ident);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::notify(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	uint event = SYSC_ARG2(stack);
	Thread *nt = Thread::getById(tid);

	if(EXPECT_FALSE(!IS_USER_NOTIFY_EVENT(event) || nt == NULL))
		SYSC_ERROR(stack,-EINVAL);
	Sched::wakeupThread(nt,event);
	SYSC_RET1(stack,0);
}

int Syscalls::lock(Thread *t,IntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	ushort flags = (uint)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::acquire(global ? INVALID_PID : pid,ident,flags);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::unlock(Thread *t,IntrptStackFrame *stack) {
	ulong ident = SYSC_ARG1(stack);
	bool global = (bool)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();

	int res = Lock::release(global ? INVALID_PID : pid,ident);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::join(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	if(tid != 0) {
		const Thread *tt = Thread::getById(tid);
		/* just threads from the own process */
		if(EXPECT_FALSE(tt == NULL || tt->getTid() == t->getTid() ||
				tt->getProc()->getPid() != t->getProc()->getPid()))
			SYSC_ERROR(stack,-EINVAL);
	}

	Proc::join(tid);
	SYSC_RET1(stack,0);
}

int Syscalls::suspend(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(EXPECT_FALSE(tt == NULL || tt->getTid() == t->getTid() ||
			tt->getProc()->getPid() != t->getProc()->getPid()))
		SYSC_ERROR(stack,-EINVAL);
	tt->suspend();
	SYSC_RET1(stack,0);
}

int Syscalls::resume(Thread *t,IntrptStackFrame *stack) {
	tid_t tid = (tid_t)SYSC_ARG1(stack);
	Thread *tt = Thread::getById(tid);
	/* just threads from the own process */
	if(EXPECT_FALSE(tt == NULL || tt->getTid() == t->getTid() ||
			tt->getProc()->getPid() != t->getProc()->getPid()))
		SYSC_ERROR(stack,-EINVAL);
	tt->unsuspend();
	SYSC_RET1(stack,0);
}

static int doWait(uint event,evobj_t object,time_t maxWaitTime,pid_t pid,ulong ident) {
	OpenFile *file = NULL;
	/* first request the files from the file-descriptors */
	if(IS_FILE_EVENT(event)) {
		/* translate fd to node-number */
		file = FileDesc::request((int)object);
		if(EXPECT_FALSE(file == NULL))
			return -EBADF;
	}

	/* now wait */
	if(EXPECT_FALSE(!IS_USER_WAIT_EVENT(event)))
		return -EINVAL;
	if(IS_FILE_EVENT(event))
		object = (evobj_t)file;
	int res = VFS::waitFor(event,object,maxWaitTime,true,pid,ident);

	/* release them */
	if(IS_FILE_EVENT(event))
		FileDesc::release(file);
	return res;
}
