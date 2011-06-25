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

#ifndef EVENT_H_
#define EVENT_H_

#include <sys/common.h>

typedef struct {
	/* the events to wait for */
	uint events;
	/* the object (0 = ignore) */
	evobj_t object;
} sWaitObject;

/* the event-indices */
#define EVI_CLIENT				0
#define EVI_RECEIVED_MSG		1
#define EVI_REQ_REPLY			2
#define EVI_DATA_READABLE		3
#define EVI_PIPE_FULL			4
#define EVI_PIPE_EMPTY			5
#define EVI_UNLOCK_SH			6
#define EVI_UNLOCK_EX			7
#define EVI_REQ_FREE			8
#define EVI_USER1				9
#define EVI_USER2				10
#define EVI_VM86_READY			11
#define EVI_SWAP_DONE			12
#define EVI_SWAP_WORK			13
#define EVI_SWAP_FREE			14
#define EVI_VMM_DONE			15
#define EVI_THREAD_DIED			16
#define EVI_CHILD_DIED			17

/* the events we can wait for */
#define EV_NOEVENT				0
#define EV_CLIENT				(1 << EVI_CLIENT)
#define EV_RECEIVED_MSG			(1 << EVI_RECEIVED_MSG)
#define EV_REQ_REPLY			(1 << EVI_REQ_REPLY)	/* kernel-intern */
#define EV_DATA_READABLE		(1 << EVI_DATA_READABLE)
#define EV_PIPE_FULL			(1 << EVI_PIPE_FULL)	/* kernel-intern */
#define EV_PIPE_EMPTY			(1 << EVI_PIPE_EMPTY)	/* kernel-intern */
#define EV_UNLOCK_SH			(1 << EVI_UNLOCK_SH)	/* kernel-intern */
#define EV_UNLOCK_EX			(1 << EVI_UNLOCK_EX)	/* kernel-intern */
#define EV_REQ_FREE				(1 << EVI_REQ_FREE)		/* kernel-intern */
#define EV_USER1				(1 << EVI_USER1)
#define EV_USER2				(1 << EVI_USER2)
#define EV_VM86_READY			(1 << EVI_VM86_READY)	/* kernel-intern */
#define EV_SWAP_DONE			(1 << EVI_SWAP_DONE)	/* kernel-intern */
#define EV_SWAP_WORK			(1 << EVI_SWAP_WORK)	/* kernel-intern */
#define EV_SWAP_FREE			(1 << EVI_SWAP_FREE)	/* kernel-intern */
#define EV_VMM_DONE				(1 << EVI_VMM_DONE)		/* kernel-intern */
#define EV_THREAD_DIED			(1 << EVI_THREAD_DIED)	/* kernel-intern */
#define EV_CHILD_DIED			(1 << EVI_CHILD_DIED)	/* kernel-intern */
#define EV_COUNT				18

/* the events a user-thread can wait for */
#define EV_USER_WAIT_MASK		(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE | \
								EV_USER1 | EV_USER2)
/* the events a user-thread can fire */
#define EV_USER_NOTIFY_MASK		(EV_USER1 | EV_USER2)

/**
 * Inits the event-system
 */
void ev_init(void);

/**
 * Checks wether the given threads waits for the given events
 *
 * @param tid the thread-id
 * @param events the event-mask (not index!)
 * @return true if so
 */
bool ev_waitsFor(tid_t tid,uint events);

/**
 * Lets <tid> wait for the given event and object
 *
 * @param tid the thread-id
 * @param evi the event-index(!)
 * @param object the object (0 = ignore)
 * @return true if successfull
 */
bool ev_wait(tid_t tid,size_t evi,evobj_t object);

/**
 * Lets <tid> wait for the given objects
 *
 * @param tid the thread-id
 * @param objects the objects to wait for
 * @param objCount the number of objects
 * @return true if successfull
 */
bool ev_waitObjects(tid_t tid,const sWaitObject *objects,size_t objCount);

/**
 * Wakes up all threads that wait for given event and object
 *
 * @param evi the event-index(!)
 * @param object the object
 */
void ev_wakeup(size_t evi,evobj_t object);

/**
 * Wakes up all threads that wait for given events and the given object
 *
 * @param events the event-mask (not index!)
 * @param object the object
 */
void ev_wakeupm(uint events,evobj_t object);

/**
 * Wakes up the thread <tid> for given events. That means, if it does not wait for them, it is
 * not waked up.
 *
 * @param tid the thread-id
 * @param events the event-mask (not index!)
 * @return true if waked up
 */
bool ev_wakeupThread(tid_t tid,uint events);

/**
 * Removes the given thread from the event-system. Note that it will set it to the ready-state!
 *
 * @param tid the thread-id
 */
void ev_removeThread(tid_t tid);

/**
 * Prints the given event-mask
 *
 * @param mask the mask
 */
void ev_printEvMask(uint mask);

/**
 * Prints all waiting threads
 */
void ev_print(void);

#endif /* EVENT_H_ */
