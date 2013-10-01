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

/* the events we can wait for */
#define EV_NOEVENT					0
#define EV_CLIENT					1
#define EV_RECEIVED_MSG				2
#define EV_DATA_READABLE			3
#define EV_MUTEX					4		/* kernel-intern */
#define EV_PIPE_FULL				5		/* kernel-intern */
#define EV_PIPE_EMPTY				6		/* kernel-intern */
#define EV_UNLOCK_SH				7		/* kernel-intern */
#define EV_UNLOCK_EX				8		/* kernel-intern */
#define EV_REQ_FREE					9		/* kernel-intern */
#define EV_USER1					10
#define EV_USER2					11
#define EV_SWAP_JOB					12		/* kernel-intern */
#define EV_SWAP_WORK				13		/* kernel-intern */
#define EV_SWAP_FREE				14		/* kernel-intern */
#define EV_VMM_DONE					15		/* kernel-intern */
#define EV_THREAD_DIED				16		/* kernel-intern */
#define EV_CHILD_DIED				17		/* kernel-intern */
#define EV_TERMINATION				18		/* kernel-intern */
#define EV_COUNT					18

#define IS_FILE_EVENT(ev)			((ev) >= EV_CLIENT && (ev) <= EV_DATA_READABLE)
#define IS_USER_NOTIFY_EVENT(ev)	((ev) == EV_USER1 || (ev) == EV_USER2)
#define IS_USER_WAIT_EVENT(ev)		((ev) == EV_NOEVENT || IS_FILE_EVENT((ev)) || \
 									 IS_USER_NOTIFY_EVENT((ev)))

class Event {
	Event() = delete;

public:
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
	 * @param event the event to wait for
	 * @param object the object (0 = ignore)
	 * @return true if successfull
	 */
	static bool wait(Thread *t,uint event,evobj_t object);

	/**
	 * Wakes up all threads that wait for given event and object
	 *
	 * @param event the event
	 * @param object the object
	 */
	static void wakeup(uint event,evobj_t object);

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
	static const char *getName(uint event);

	static klock_t lock;
	static DList<Thread> evlists[EV_COUNT];
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
