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
#include <sys/task/sched.h>
#include <sys/mem/kheap.h>
#include <sys/util.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

/**
 * We're using round-robin here atm. That means a thread-switch puts the current thread at the end
 * of the ready-queue and makes the first in the ready-queue the new running thread.
 * Additionally we can block threads so that they lie on the blocked-queue until they are unblocked
 * by the kernel.
 *
 * Each thread has a prev- and next-pointer with which we build two double-linked list: one
 * ready-queue and one blocked-queue. For both we store the beginning and end.
 * Therefore we can dequeue the first, prepend, append and remove a thread in O(1).
 * Additionally the number of threads is limited by the kernel-heap (i.e. we don't need a static
 * storage of nodes for the linked list; we use the threads itself)
 */

typedef struct {
	sThread *first;
	sThread *last;
} sQueue;

static bool sched_setReadyState(sThread *t);
static void sched_qInit(sQueue *q);
static sThread *sched_qDequeue(sQueue *q);
static void sched_qDequeueThread(sQueue *q,sThread *t);
static void sched_qAppend(sQueue *q,sThread *t);
static void sched_qPrint(sQueue *q);

static klock_t schedLock;
static sQueue readyQueue;

void sched_init(void) {
	sched_qInit(&readyQueue);
}

sThread *sched_perform(sThread *old) {
	sThread *t;
	klock_aquire(&schedLock);
	/* give the old thread a new state */
	if(old) {
		if(old->flags & T_IDLE)
			thread_pushIdle(old);
		else {
			assert(old->state == ST_RUNNING || old->state == ST_ZOMBIE);
			/* we have to check for a signal here, because otherwise we might miss it */
			/* (scenario: cpu0 unblocks t1 for signal, cpu1 runs t1 and blocks itself) */
			if(sig_hasSignalFor(old->tid)) {
				klock_release(&schedLock);
				return old;
			}
			switch(old->newState) {
				case ST_READY:
					old->state = ST_READY;
					sched_qAppend(&readyQueue,old);
					break;
				case ST_BLOCKED:
					old->state = ST_BLOCKED;
					break;
				case ST_ZOMBIE:
					break;
				default:
					util_panic("Unexpected new state (%d)\n",old->newState);
					break;
			}
		}
	}

	/* get new thread */
	t = sched_qDequeue(&readyQueue);
	if(t == NULL) {
		/* choose an idle-thread */
		t = thread_popIdle();
	}
	else {
		t->state = ST_RUNNING;
		t->newState = ST_READY;
	}

	klock_release(&schedLock);
	return t;
}

void sched_setReady(sThread *t) {
	assert(t != NULL);
	if(t->flags & T_IDLE)
		return;

	klock_aquire(&schedLock);
	if(t->state == ST_RUNNING)
		t->newState = ST_READY;
	else {
		if(sched_setReadyState(t))
			sched_qAppend(&readyQueue,t);
	}
	klock_release(&schedLock);
}

static bool sched_setReadyState(sThread *t) {
	switch(t->state) {
		case ST_READY:
		case ST_READY_SUSP:
			return false;
		case ST_BLOCKED_SUSP:
			t->state = ST_READY_SUSP;
			return false;
		case ST_BLOCKED:
			break;
		default:
			vassert(false,"Invalid state for setReady (%d)",t->state);
			break;
	}
	t->state = ST_READY;
	return true;
}

void sched_setBlocked(sThread *t) {
	assert(t != NULL);
	klock_aquire(&schedLock);
	switch(t->state) {
		case ST_BLOCKED:
		case ST_BLOCKED_SUSP:
			break;
		case ST_READY_SUSP:
			t->state = ST_BLOCKED_SUSP;
			break;
		case ST_RUNNING:
			t->newState = ST_BLOCKED;
			break;
		case ST_READY:
			t->state = ST_BLOCKED;
			sched_qDequeueThread(&readyQueue,t);
			break;
		default:
			vassert(false,"Invalid state for setBlocked (%d)",t->state);
			break;
	}
	klock_release(&schedLock);
}

void sched_setSuspended(sThread *t,bool blocked) {
	assert(t != NULL);

	klock_aquire(&schedLock);
	if(blocked) {
		switch(t->state) {
			/* already suspended, so ignore it */
			case ST_BLOCKED_SUSP:
			case ST_ZOMBIE_SUSP:
			case ST_READY_SUSP:
				break;
			case ST_BLOCKED:
				t->state = ST_BLOCKED_SUSP;
				break;
			case ST_ZOMBIE:
				t->state = ST_ZOMBIE_SUSP;
				break;
			case ST_READY:
				t->state = ST_READY_SUSP;
				sched_qDequeueThread(&readyQueue,t);
				break;
			default:
				vassert(false,"Thread %d has invalid state for suspending (%d)",t->tid,t->state);
				break;
		}
	}
	else {
		switch(t->state) {
			/* not suspended, so ignore it */
			case ST_BLOCKED:
			case ST_ZOMBIE:
			case ST_READY:
				break;
			case ST_BLOCKED_SUSP:
				t->state = ST_BLOCKED;
				break;
			case ST_ZOMBIE_SUSP:
				t->state = ST_ZOMBIE;
				break;
			case ST_READY_SUSP:
				t->state = ST_READY;
				sched_qAppend(&readyQueue,t);
				break;
			default:
				vassert(false,"Thread %d has invalid state for resuming (%d)",t->tid,t->state);
				break;
		}
	}
	klock_release(&schedLock);
}

void sched_removeThread(sThread *t) {
	klock_aquire(&schedLock);
	switch(t->state) {
		case ST_RUNNING:
		case ST_ZOMBIE:
		case ST_BLOCKED:
			break;
		case ST_READY:
			sched_qDequeueThread(&readyQueue,t);
			break;
		default:
			/* TODO threads can die during swap, right? */
			vassert(false,"Invalid state for removeThread (%d)",t->state);
			break;
	}
	t->state = ST_ZOMBIE;
	t->newState = ST_ZOMBIE;
	klock_release(&schedLock);
}

void sched_print(void) {
	vid_printf("Ready-");
	sched_qPrint(&readyQueue);
	vid_printf("\n");
}

static void sched_qInit(sQueue *q) {
	q->first = NULL;
	q->last = NULL;
}

static sThread *sched_qDequeue(sQueue *q) {
	sThread *t = q->first;
	if(t == NULL)
		return NULL;

	if(t->next)
		t->next->prev = NULL;
	if(t == q->last)
		q->last = NULL;
	q->first = t->next;
	return t;
}

static void sched_qDequeueThread(sQueue *q,sThread *t) {
	if(q->first == t)
		q->first = t->next;
	else
		t->prev->next = t->next;
	if(q->last == t)
		q->last = t->prev;
	else
		t->next->prev = t->prev;
}

static void sched_qAppend(sQueue *q,sThread *t) {
	t->prev = q->last;
	t->next = NULL;
	if(t->prev)
		t->prev->next = t;
	else
		q->first = t;
	q->last = t;
}

static void sched_qPrint(sQueue *q) {
	char name1[12];
	char name2[12];
	const sThread *t = q->first;
	vid_printf("Queue: first=%s, last=%s\n",
			q->first ? itoa(name1,sizeof(name1),q->first->tid) : "-",
			q->last ? itoa(name2,sizeof(name2),q->last->tid) : "-");
	while(t != NULL) {
		vid_printf("\ttid=%d, prev=%s, next=%s\n",
				t->tid,t->prev ? itoa(name1,sizeof(name1),t->prev->tid) : "-",
				t->next ? itoa(name2,sizeof(name2),t->next->tid) : "-");
		t = t->next;
	}
}
