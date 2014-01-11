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

#pragma once

#include <sys/common.h>
#include <sys/col/islist.h>
#include <sys/col/dlist.h>
#include <sys/spinlock.h>

/* the events we can wait for */
#define EV_NOEVENT					0
#define EV_CLIENT					1
#define EV_RECEIVED_MSG				2
#define EV_DATA_READABLE			3
#define EV_MUTEX					4		/* kernel-intern */
#define EV_PIPE_FULL				5		/* kernel-intern */
#define EV_PIPE_EMPTY				6		/* kernel-intern */
#define EV_REQ_FREE					7		/* kernel-intern */
#define EV_USER1					8
#define EV_USER2					9
#define EV_SWAP_JOB					10		/* kernel-intern */
#define EV_SWAP_WORK				11		/* kernel-intern */
#define EV_SWAP_FREE				12		/* kernel-intern */
#define EV_VMM_DONE					13		/* kernel-intern */
#define EV_THREAD_DIED				14		/* kernel-intern */
#define EV_CHILD_DIED				15		/* kernel-intern */
#define EV_TERMINATION				16		/* kernel-intern */
#define EV_COUNT					17

#define IS_FILE_EVENT(ev)			((ev) >= EV_CLIENT && (ev) <= EV_DATA_READABLE)
#define IS_USER_NOTIFY_EVENT(ev)	((ev) == EV_USER1 || (ev) == EV_USER2)
#define IS_USER_WAIT_EVENT(ev)		((ev) == EV_NOEVENT || IS_FILE_EVENT((ev)) || \
 									 IS_USER_NOTIFY_EVENT((ev)))

class Thread;
class ThreadBase;
class OStream;

class Sched {
	friend class Thread;
	friend class ThreadBase;

	Sched() = delete;

public:
	/**
	 * Inits the scheduler
	 */
	static void init();

	/**
	 * Lets <tid> wait for the given event and object
	 *
	 * @param t the thread
	 * @param event the event to wait for
	 * @param object the object (0 = ignore)
	 */
	static void wait(Thread *t,uint event,evobj_t object);

	/**
	 * Wakes up all threads that wait for given event and object
	 *
	 * @param event the event
	 * @param object the object
	 * @param all if true, all are waked up, otherwise only the first one
	 */
	static void wakeup(uint event,evobj_t object,bool all = true);

	/**
	 * Wakes up the thread <t> for given event. That means, if it does not wait for it, it is
	 * not waked up.
	 *
	 * @param t the thread
	 * @param event the event
	 * @return true if waked up
	 */
	static bool wakeupThread(Thread *t,uint event);

	/**
	 * @return the current ready-mask. 1 bit per priority.
	 */
	static ulong getReadyMask() {
		return readyMask;
	}

	/**
	 * Blocks the given thread
	 *
	 * @param t the thread
	 */
	static void block(Thread *t);

	/**
	 * Unblocks the given thread
	 *
	 * @param t the thread
	 */
	static void unblock(Thread *t);

	/**
	 * Unblocks the given thread and puts it to the beginning of the ready-list
	 *
	 * @param t the thread
	 */
	static void unblockQuick(Thread *t);

	/**
	 * Suspends the given thread
	 *
	 * @param t the thread
	 */
	static void suspend(Thread *t);

	/**
	 * Resumes the given thread
	 *
	 * @param t the thread
	 */
	static void unsuspend(Thread *t);

	/**
	 * Prints the status of the scheduler
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

	/**
	 * Prints the event-lists
	 *
	 * @param os the output-stream
	 */
	static void printEventLists(OStream &os);

	/**
	 * @param event the event
	 * @return the name of that event
	 */
	static const char *getEventName(uint event);

private:
	/**
	 * Adds the given thread as an idle-thread to the scheduler
	 *
	 * @param t the thread
	 */
	static void addIdleThread(Thread *t);

	/**
	 * Performs the scheduling. That means it picks the next thread to run and returns it
	 *
	 * @param old the current thread
	 * @param cpu the CPU
	 * @return the thread to run
	 */
	static Thread *perform(Thread *old,cpuid_t cpu);

	/**
	 * Adjusts the priority of the given thread according to its used timeslice
	 *
	 * @param t the thread
	 * @param total total number of cycles executed (per CPU)
	 */
	static void adjustPrio(Thread *t,uint64_t total);

	/**
	 * Appends the given thread on the ready-queue and sets the state to Thread::READY
	 *
	 * @param t the thread
	 */
	static void setReady(Thread *t);

	/**
	 * Puts the given thread to the beginning of the ready-queue
	 *
	 * @param t the thread
	 */
	static void setReadyQuick(Thread *t);

	/**
	 * Sets the thread in the blocked-state
	 *
	 * @param t the thread
	 */
	static void setBlocked(Thread *t);

	/**
	 * Suspends / resumes the given thread. That means it will be put into a state
	 * that ensures that it can't be chosen until its resumed.
	 *
	 * @param t the thread
	 * @param blocked whether to block or unblock it
	 */
	static void setSuspended(Thread *t,bool blocked);

	/**
	 * Removes the given thread from the scheduler (depending on the state)
	 *
	 * @param t the thread
	 */
	static void removeThread(Thread *t);

	static void enqueue(Thread *t);
	static void enqueueQuick(Thread *t);
	static void dequeue(Thread *t);
	static void removeFromEventlist(Thread *t);
	static bool setReadyState(Thread *t);
	static void print(OStream &os,DList<Thread> *q);

	static klock_t lock;
	static ulong readyMask;
	static DList<Thread> rdyQueues[];
	static DList<Thread> evlists[EV_COUNT];
	static size_t rdyCount;
	static Thread **idleThreads;
};

inline void Sched::block(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	setBlocked(t);
	SpinLock::release(&lock);
}

inline void Sched::unblock(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	setReady(t);
	SpinLock::release(&lock);
}

inline void Sched::unblockQuick(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	setReadyQuick(t);
	SpinLock::release(&lock);
}

inline void Sched::suspend(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	setSuspended(t,true);
	SpinLock::release(&lock);
}

inline void Sched::unsuspend(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	setSuspended(t,false);
	SpinLock::release(&lock);
}
