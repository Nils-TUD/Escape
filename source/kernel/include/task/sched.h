/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/col/dlist.h>
#include <esc/col/islist.h>
#include <common.h>
#include <lockguard.h>
#include <spinlock.h>

/* the events we can wait for */
enum {
	EV_NOEVENT,
	EV_CLIENT,
	EV_RECEIVED_MSG,
	EV_MUTEX,
	EV_SWAP_JOB,
	EV_SWAP_WORK,
	EV_SWAP_FREE,
	EV_THREAD_DIED,
	EV_CHILD_DIED,
	EV_COUNT = EV_CHILD_DIED,
};

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
	static void print(OStream &os,esc::DList<Thread> *q);

	static SpinLock lock;
	static ulong readyMask;
	static esc::DList<Thread> rdyQueues[];
	static esc::DList<Thread> evlists[EV_COUNT];
	static size_t rdyCount;
	static Thread **idleThreads;
};

inline void Sched::block(Thread *t) {
	assert(t != NULL);
	LockGuard<SpinLock> g(&lock);
	setBlocked(t);
}

inline void Sched::unblock(Thread *t) {
	assert(t != NULL);
	LockGuard<SpinLock> g(&lock);
	setReady(t);
}

inline void Sched::unblockQuick(Thread *t) {
	assert(t != NULL);
	LockGuard<SpinLock> g(&lock);
	setReadyQuick(t);
}
