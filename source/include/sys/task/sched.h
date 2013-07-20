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
#include <esc/sllist.h>

class Thread;
class ThreadBase;

class Sched {
	friend class Thread;
	friend class ThreadBase;

	Sched() = delete;

public:
	struct Queue {
		Thread *first;
		Thread *last;
	};

	/**
	 * Inits the scheduler
	 */
	static void init();

	/**
	 * Prints the status of the scheduler
	 */
	static void print();

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
	 * @param runtime the runtime of the current thread in microseconds
	 * @return the thread to run
	 */
	static Thread *perform(Thread *old,uint64_t runtime);

	/**
	 * Adjusts the priority of the given thread according to its used timeslice
	 *
	 * @param t the thread
	 * @param threadCount total number of threads
	 */
	static void adjustPrio(Thread *t,size_t threadCount);

	/**
	 * Appends the given thread on the ready-queue and sets the state to Thread::READY
	 *
	 * @param t the thread
	 * @return true if successfull (i.e. if the thread is ready now and wasn't previously)
	 */
	static bool setReady(Thread *t);

	/**
	 * Puts the given thread to the beginning of the ready-queue
	 *
	 * @param t the thread
	 * @return true if successfull (i.e. if the thread is ready now and wasn't previously)
	 */
	static bool setReadyQuick(Thread *t);

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

	static bool setReadyState(Thread *t);
	static void qInit(Queue *q);
	static Thread *qDequeue(Queue *q);
	static void qDequeueThread(Queue *q,Thread *t);
	static void qAppend(Queue *q,Thread *t);
	static void qPrepend(Queue *q,Thread *t);
	static void qPrint(Queue *q);

	static klock_t schedLock;
	static Queue rdyQueues[];
	static size_t rdyCount;
	static sSLList idleThreads;
};
