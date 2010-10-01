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
	u32 events;
	void *object;
} sWaitObject;

#define EVI_CLIENT				0
#define EVI_RECEIVED_MSG		1
#define EVI_CHILD_DIED			2
#define EVI_DATA_READABLE		3
#define EVI_UNLOCK_SH			4
#define EVI_PIPE_FULL			5
#define EVI_PIPE_EMPTY			6
#define EVI_VM86_READY			7
#define EVI_REQ_REPLY			8
#define EVI_SWAP_DONE			9
#define EVI_SWAP_WORK			10
#define EVI_SWAP_FREE			11
#define EVI_VMM_DONE			12
#define EVI_THREAD_DIED			13
#define EVI_USER1				14
#define EVI_USER2				15
#define EVI_REQ_FREE			16
#define EVI_UNLOCK_EX			17

/* the events we can wait for */
#define EV_NOEVENT				0
#define EV_CLIENT				(1 << EVI_CLIENT)
#define EV_RECEIVED_MSG			(1 << EVI_RECEIVED_MSG)
#define EV_CHILD_DIED			(1 << EVI_CHILD_DIED)	/* kernel-intern */
#define EV_DATA_READABLE		(1 << EVI_DATA_READABLE)
#define EV_UNLOCK_SH			(1 << EVI_UNLOCK_SH)	/* kernel-intern */
#define EV_PIPE_FULL			(1 << EVI_PIPE_FULL)	/* kernel-intern */
#define EV_PIPE_EMPTY			(1 << EVI_PIPE_EMPTY)	/* kernel-intern */
#define EV_VM86_READY			(1 << EVI_VM86_READY)	/* kernel-intern */
#define EV_REQ_REPLY			(1 << EVI_REQ_REPLY)	/* kernel-intern */
#define EV_SWAP_DONE			(1 << EVI_SWAP_DONE)	/* kernel-intern */
#define EV_SWAP_WORK			(1 << EVI_SWAP_WORK)	/* kernel-intern */
#define EV_SWAP_FREE			(1 << EVI_SWAP_FREE)	/* kernel-intern */
#define EV_VMM_DONE				(1 << EVI_VMM_DONE)		/* kernel-intern */
#define EV_THREAD_DIED			(1 << EVI_THREAD_DIED)	/* kernel-intern */
#define EV_USER1				(1 << EVI_USER1)
#define EV_USER2				(1 << EVI_USER2)
#define EV_REQ_FREE				(1 << EVI_REQ_FREE)		/* kernel-intern */
#define EV_UNLOCK_EX			(1 << EVI_UNLOCK_EX)	/* kernel-intern */
#define EV_COUNT				18

/* the events a user-thread can wait for */
#define EV_USER_WAIT_MASK		(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE | \
								EV_USER1 | EV_USER2)
/* the events a user-thread can fire */
#define EV_USER_NOTIFY_MASK		(EV_USER1 | EV_USER2)

void ev_init(void);
bool ev_wait(tTid tid,u32 evi,void *object);
bool ev_waitm(tTid tid,u32 events);
bool ev_waitObjects(tTid tid,sWaitObject *objects,u32 objCount);
void ev_wakeup(u32 evi,void *object);
bool ev_wakeupThread(tTid tid,u32 events);
void ev_removeThread(tTid tid);

#if DEBUGGING

void ev_dbg_print(void);

#endif

#endif /* EVENT_H_ */
