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
#include <sys/task/thread.h>
#include <sys/spinlock.h>

/* the event-indices */
#define EVI_CLIENT				0
#define EVI_RECEIVED_MSG		1
#define EVI_DATA_READABLE		2
#define EVI_MUTEX				3
#define EVI_PIPE_FULL			4
#define EVI_PIPE_EMPTY			5
#define EVI_UNLOCK_SH			6
#define EVI_UNLOCK_EX			7
#define EVI_REQ_FREE			8
#define EVI_USER1				9
#define EVI_USER2				10
#define EVI_SWAP_JOB			11
#define EVI_SWAP_WORK			12
#define EVI_SWAP_FREE			13
#define EVI_VMM_DONE			14
#define EVI_THREAD_DIED			15
#define EVI_CHILD_DIED			16
#define EVI_TERMINATION			17

/* the events we can wait for */
#define EV_NOEVENT				0
#define EV_CLIENT				(1 << EVI_CLIENT)
#define EV_RECEIVED_MSG			(1 << EVI_RECEIVED_MSG)
#define EV_DATA_READABLE		(1 << EVI_DATA_READABLE)
#define EV_MUTEX				(1 << EVI_MUTEX)		/* kernel-intern */
#define EV_PIPE_FULL			(1 << EVI_PIPE_FULL)	/* kernel-intern */
#define EV_PIPE_EMPTY			(1 << EVI_PIPE_EMPTY)	/* kernel-intern */
#define EV_UNLOCK_SH			(1 << EVI_UNLOCK_SH)	/* kernel-intern */
#define EV_UNLOCK_EX			(1 << EVI_UNLOCK_EX)	/* kernel-intern */
#define EV_REQ_FREE				(1 << EVI_REQ_FREE)		/* kernel-intern */
#define EV_USER1				(1 << EVI_USER1)
#define EV_USER2				(1 << EVI_USER2)
#define EV_SWAP_JOB				(1 << EVI_SWAP_JOB)		/* kernel-intern */
#define EV_SWAP_WORK			(1 << EVI_SWAP_WORK)	/* kernel-intern */
#define EV_SWAP_FREE			(1 << EVI_SWAP_FREE)	/* kernel-intern */
#define EV_VMM_DONE				(1 << EVI_VMM_DONE)		/* kernel-intern */
#define EV_THREAD_DIED			(1 << EVI_THREAD_DIED)	/* kernel-intern */
#define EV_CHILD_DIED			(1 << EVI_CHILD_DIED)	/* kernel-intern */
#define EV_TERMINATION			(1 << EVI_TERMINATION)	/* kernel-intern */
#define EV_COUNT				18

/* the events a user-thread can wait for */
#define EV_USER_WAIT_MASK		(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE | \
								EV_USER1 | EV_USER2)
/* the events a user-thread can fire */
#define EV_USER_NOTIFY_MASK		(EV_USER1 | EV_USER2)

class Event {
	Event() = delete;

	static const size_t MAX_WAIT_COUNT		= 2048;
	static const size_t MAX_WAKEUPS			= 8;

	struct WaitList {
		sWait *begin;
		sWait *last;
	};

public:
	struct WaitObject {
		/* the events to wait for */
		uint events;
		/* the object (0 = ignore) */
		evobj_t object;
	};

	/**
	 * Inits the event-system
	 */
	static void init();

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
	 * Lets <tid> wait for the given event and object
	 *
	 * @param t the thread
	 * @param evi the event-index(!)
	 * @param object the object (0 = ignore)
	 * @return true if successfull
	 */
	static bool wait(Thread *t,size_t evi,evobj_t object);

	/**
	 * Lets <tid> wait for the given objects
	 *
	 * @param t the thread
	 * @param objects the objects to wait for
	 * @param objCount the number of objects
	 * @return true if successfull
	 */
	static bool waitObjects(Thread *t,const WaitObject *objects,size_t objCount);

	/**
	 * Wakes up all threads that wait for given event and object
	 *
	 * @param evi the event-index(!)
	 * @param object the object
	 */
	static void wakeup(size_t evi,evobj_t object);

	/**
	 * Wakes up all threads that wait for given events and the given object
	 *
	 * @param events the event-mask (not index!)
	 * @param object the object
	 */
	static void wakeupm(uint events,evobj_t object);

	/**
	 * Wakes up the thread <tid> for given events. That means, if it does not wait for them, it is
	 * not waked up.
	 *
	 * @param t the thread
	 * @param events the event-mask (not index!)
	 * @return true if waked up
	 */
	static bool wakeupThread(Thread *t,uint events);

	/**
	 * Removes the given thread from the event-system. Note that it will set it to the ready-state!
	 *
	 * @param t the thread
	 */
	static void removeThread(Thread *t);

	/**
	 * Prints the event-mask of given thread
	 *
	 * @param os the output-stream
	 * @param t the thread
	 */
	static void printEvMask(OStream &os,const Thread *t);

	/**
	 * Prints all waiting threads
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	static void doRemoveThread(Thread *t);
	static sWait *doWait(Thread *t,size_t evi,evobj_t object,sWait **begin,sWait *prev);
	static sWait *allocWait();
	static void freeWait(sWait *w);
	static const char *getName(size_t evi);

	static klock_t lock;
	static sWait waits[MAX_WAIT_COUNT];
	static sWait *waitFree;
	static WaitList evlists[EV_COUNT];
};

inline void Event::block(Thread *t) {
	t->block();
}

inline void Event::unblock(Thread *t) {
	removeThread(t);
	t->unblock();
}

inline void Event::unblockQuick(Thread *t) {
	removeThread(t);
	t->unblockQuick();
}

inline void Event::suspend(Thread *t) {
	t->suspend();
}

inline void Event::unsuspend(Thread *t) {
	t->unsuspend();
}

inline void Event::removeThread(Thread *t) {
	SpinLock::acquire(&lock);
	doRemoveThread(t);
	SpinLock::release(&lock);
}
