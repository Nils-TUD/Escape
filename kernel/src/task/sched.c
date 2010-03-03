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

#include <common.h>
#include <task/thread.h>
#include <task/sched.h>
#include <util.h>
#include <video.h>
#include <sllist.h>
#include <assert.h>

/**
 * The scheduling-algorithm is very simple atm. Basically it is round-robin.
 * That means a thread-switch puts the current thread at the end of the ready-queue and makes
 * the first in the ready-queue the new running thread.
 * Additionally we can block threads so that they lie on the blocked-queue until they
 * are unblocked by the kernel.
 * There is one special thing: threads that have been waked up because of an event or because
 * the timer noticed that the thread no longer wants to sleep, will be put at the beginning
 * of the ready-queue so that they will be chosen on the next resched.
 *
 * Note that we're using our own linked-list-implementation here instead of the SLL because we
 * don't want to waste time with allocating and freeing stuff on the heap.
 * Since we're storing the beginning and end of the list we can schedule in O(1). But changing
 * the thread-state (ready <-> blocked) takes a bit more because we have to search
 * in the queue.
 */

/* a queue-node */
typedef struct sQueueNode sQueueNode;
struct sQueueNode {
	sQueueNode *next;
	sThread *t;
};

/* all stuff we need for a queue */
typedef struct {
	sQueueNode nodes[THREAD_COUNT];
	sQueueNode *free;
	sQueueNode *first;
	sQueueNode *last;
} sQueue;

static void sched_qInit(sQueue *q);
static sThread *sched_qDequeue(sQueue *q);
static void sched_qDequeueThread(sQueue *q,sThread *t);
static void sched_qAppend(sQueue *q,sThread *t);
static void sched_qPrepend(sQueue *q,sThread *t);

/* the queues */
static sQueue readyQueue;
static sQueue blockedQueue;

void sched_init(void) {
	sched_qInit(&readyQueue);
	sched_qInit(&blockedQueue);
}

sThread *sched_perform(void) {
	sThread *t = thread_getRunning();
	bool isZombie = t->state == ST_ZOMBIE;
	if(t->state == ST_RUNNING) {
		/* put current in the ready-queue */
		sched_setReady(t);
	}

	/* get new thread */
	t = sched_qDequeue(&readyQueue);

	if(t == NULL) {
		if(isZombie) {
			/* If the current thread should be destroyed the process may be destroyed, too. That means
			 * that the kernel-stack would be free'd. So if we idle with that kernel-stack we have a
			 * problem...
			 * Therefore we choose the other thread of init to be sure (this may not be destroyed).
			 */
			t = thread_getById(INIT_TID);
		}
		else {
			/* otherwise choose the idle-thread */
			t = thread_getById(IDLE_TID);
		}
	}
	else
		t->state = ST_RUNNING;

	return t;
}

void sched_setRunning(sThread *t) {
	vassert(t != NULL,"p == NULL");

	switch(t->state) {
		case ST_RUNNING:
			return;
		case ST_READY:
			sched_qDequeueThread(&readyQueue,t);
			break;
		case ST_BLOCKED:
			sched_qDequeueThread(&blockedQueue,t);
			break;
	}

	t->state = ST_RUNNING;
}

void sched_setReady(sThread *t) {
	vassert(t != NULL,"p == NULL");

	/* nothing to do? */
	if(t->state == ST_READY)
		return;
	/* remove from blocked-list? */
	if(t->state == ST_BLOCKED)
		sched_qDequeueThread(&blockedQueue,t);
	t->state = ST_READY;

	sched_qAppend(&readyQueue,t);
}

void sched_setReadyQuick(sThread *t) {
	vassert(t != NULL,"p == NULL");

	/* nothing to do? */
	if(t->state == ST_READY)
		return;
	/* remove from blocked-list? */
	if(t->state == ST_BLOCKED)
		sched_qDequeueThread(&blockedQueue,t);
	t->state = ST_READY;

	sched_qPrepend(&readyQueue,t);
}

void sched_setBlocked(sThread *t) {
	vassert(t != NULL,"p == NULL");

	/* nothing to do? */
	if(t->state == ST_BLOCKED)
		return;
	if(t->state == ST_READY)
		sched_qDequeueThread(&readyQueue,t);
	t->state = ST_BLOCKED;

	/* insert in blocked-list */
	sched_qAppend(&blockedQueue,t);
}

void sched_unblockAll(u16 mask,u16 event) {
	sQueueNode *n,*prev,*tmp;
	sThread *t;
	u16 tmask;

	prev = NULL;
	n = blockedQueue.first;
	while(n != NULL) {
		t = (sThread*)n->t;
		tmask = t->events >> 16;
		if((tmask == 0 || tmask == mask) && (t->events & event)) {
			t->state = ST_READY;
			t->events = EV_NOEVENT;
			sched_qAppend(&readyQueue,t);

			/* dequeue */
			if(prev == NULL)
				tmp = blockedQueue.first = n->next;
			else {
				tmp = prev;
				prev->next = n->next;
			}
			if(n->next == NULL)
				blockedQueue.last = tmp;

			/* put n on the free-list and continue */
			tmp = n->next;
			n->next = blockedQueue.free;
			blockedQueue.free = n;
			n = tmp;
		}
		else {
			prev = n;
			n = n->next;
		}
	}
}

void sched_removeThread(sThread *t) {
	switch(t->state) {
		case ST_READY:
			sched_qDequeueThread(&readyQueue,t);
			break;
		case ST_BLOCKED:
			sched_qDequeueThread(&blockedQueue,t);
			break;
	}
}

static void sched_qInit(sQueue *q) {
	s32 i;
	sQueueNode *node;
	/* put all on the free-queue */
	node = &q->nodes[THREAD_COUNT - 1];
	node->next = NULL;
	node--;
	for(i = THREAD_COUNT - 2; i >= 0; i--) {
		node->next = node + 1;
		node--;
	}
	/* all free atm */
	q->free = &q->nodes[0];
	q->first = NULL;
	q->last = NULL;
}

static sThread *sched_qDequeue(sQueue *q) {
	sQueueNode *node;
	if(q->first == NULL)
		return NULL;

	/* put in free-queue & remove from queue */
	node = q->first;
	q->first = q->first->next;
	if(q->first == NULL)
		q->last = NULL;
	node->next = q->free;
	q->free = node;

	return node->t;
}

static void sched_qDequeueThread(sQueue *q,sThread *t) {
	sQueueNode *n = q->first,*l = NULL;
	vassert(t != NULL,"t == NULL");

	while(n != NULL) {
		/* found it? */
		if(n->t == t) {
			/* dequeue */
			if(l == NULL)
				l = q->first = n->next;
			else
				l->next = n->next;
			if(n->next == NULL)
				q->last = l;
			n->next = q->free;
			q->free = n;
			return;
		}
		/* to next */
		l = n;
		n = n->next;
	}
}

static void sched_qAppend(sQueue *q,sThread *t) {
	sQueueNode *nn,*n;
	vassert(t != NULL,"p == NULL");
	vassert(q->free != NULL,"No free slots in the queue!?");

	/* use first free node */
	nn = q->free;
	q->free = q->free->next;
	nn->t = t;
	nn->next = NULL;

	/* put at the end of the queue */
	n = q->first;
	if(n != NULL) {
		q->last->next = nn;
		q->last = nn;
	}
	else {
		q->first = nn;
		q->last = nn;
	}
}

static void sched_qPrepend(sQueue *q,sThread *t) {
	sQueueNode *nn;
	vassert(t != NULL,"p == NULL");

	/* use first free node */
	nn = q->free;
	q->free = q->free->next;
	nn->t = t;
	/* put at the beginning of the queue */
	nn->next = q->first;
	q->first = nn;
	if(q->last == NULL)
		q->last = nn;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

static void sched_qPrint(sQueue *q) {
	sQueueNode *n = q->first;
	vid_printf("Queue: first=0x%x, last=0x%x, free=0x%x\n",q->first,q->last,q->free);
	while(n != NULL) {
		vid_printf("\t[0x%x]: t=0x%x, tid=%d, next=0x%x\n",n,n->t,n->t->tid,n->next);
		n = n->next;
	}
}

void sched_dbg_print(void) {
	vid_printf("Ready-");
	sched_qPrint(&readyQueue);
	vid_printf("Blocked-");
	sched_qPrint(&blockedQueue);
	vid_printf("\n");
}

#endif
