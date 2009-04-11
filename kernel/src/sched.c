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
#include <sched.h>
#include <proc.h>
#include <util.h>
#include <video.h>
#include <sllist.h>
#include <assert.h>

/**
 * The scheduling-algorithm is very simple atm. Basically it is round-robin.
 * That means a process-switch puts the current process at the end of the ready-queue and makes
 * the first in the ready-queue the new running process.
 * Additionally we can block processes so that they lie on the blocked-queue until they
 * are unblocked by the kernel.
 * There is one special thing: Processes that have been waked up because of an event or because
 * the timer noticed that the process no longer wants to sleep, will be put at the beginning
 * of the ready-queue so that they will be chosen on the next resched.
 *
 * Note that we're using our own linked-list-implementation here instead of the SLL because we
 * don't want to waste time with allocating and freeing stuff on the heap.
 * Since we're storing the beginning and end of the list we can schedule in O(1). But changing
 * the process-state (ready <-> blocked) takes a bit more because we have to search
 * in the queue.
 */

/* a queue-node */
typedef struct sQueueNode sQueueNode;
struct sQueueNode {
	sQueueNode *next;
	sProc *p;
};

/* all stuff we need for a queue */
typedef struct {
	sQueueNode nodes[PROC_COUNT];
	sQueueNode *free;
	sQueueNode *first;
	sQueueNode *last;
} sQueue;

static void sched_qInit(sQueue *q);
static sProc *sched_qDequeue(sQueue *q);
static void sched_qDequeueProc(sQueue *q,sProc *p);
static void sched_qAppend(sQueue *q,sProc *p);
static void sched_qPrepend(sQueue *q,sProc *p);

/* the queues */
static sQueue readyQueue;
static sQueue blockedQueue;

void sched_init(void) {
	sched_qInit(&readyQueue);
	sched_qInit(&blockedQueue);
}

sProc *sched_perform(void) {
	sProc *p = proc_getRunning();
	if(p->state == ST_RUNNING) {
		/* put current in the ready-queue */
		sched_setReady(p);
	}

	/* get new process */
	p = sched_qDequeue(&readyQueue);

	if(p == NULL) {
		/* if there is no runnable process, choose init */
		/* init will go to wait as soon as it is resumed. so if there are other processes
		 * that want to run, they will be chosen and init will never be chosen. If there is no
		 * runnable process anymore, we will choose init until there is one again. */
		p = proc_getByPid(0);
		sched_setRunning(p);
	}
	else
		p->state = ST_RUNNING;

	return p;
}

void sched_setRunning(sProc *p) {
	vassert(p != NULL,"p == NULL");

	switch(p->state) {
		case ST_RUNNING:
			return;
		case ST_READY:
			sched_qDequeueProc(&readyQueue,p);
			break;
		case ST_BLOCKED:
			sched_qDequeueProc(&blockedQueue,p);
			break;
	}

	p->state = ST_RUNNING;
}

void sched_setReady(sProc *p) {
	vassert(p != NULL,"p == NULL");

	/* nothing to do? */
	if(p->state == ST_READY)
		return;
	/* remove from blocked-list? */
	if(p->state == ST_BLOCKED)
		sched_qDequeueProc(&blockedQueue,p);
	p->state = ST_READY;

	sched_qAppend(&readyQueue,p);
}

void sched_setReadyQuick(sProc *p) {
	vassert(p != NULL,"p == NULL");

	/* nothing to do? */
	if(p->state == ST_READY)
		return;
	/* remove from blocked-list? */
	if(p->state == ST_BLOCKED)
		sched_qDequeueProc(&blockedQueue,p);
	p->state = ST_READY;

	sched_qPrepend(&readyQueue,p);
}

void sched_setBlocked(sProc *p) {
	vassert(p != NULL,"p == NULL");

	/* nothing to do? */
	if(p->state == ST_BLOCKED)
		return;
	if(p->state == ST_READY)
		sched_qDequeueProc(&readyQueue,p);
	p->state = ST_BLOCKED;

	/* insert in blocked-list */
	sched_qAppend(&blockedQueue,p);
}

void sched_unblockAll(u8 event) {
	sQueueNode *n,*prev,*t;
	sProc *p;

	prev = NULL;
	n = blockedQueue.first;
	while(n != NULL) {
		p = (sProc*)n->p;
		if(p->events & event) {
			p->state = ST_READY;
			p->events = EV_NOEVENT;
			sched_qAppend(&readyQueue,p);

			/* dequeue */
			if(prev == NULL)
				t = blockedQueue.first = n->next;
			else {
				t = prev;
				prev->next = n->next;
			}
			if(n->next == NULL)
				blockedQueue.last = t;

			/* put n on the free-list and continue */
			t = n->next;
			n->next = blockedQueue.free;
			blockedQueue.free = n;
			n = t;
		}
		else {
			prev = n;
			n = n->next;
		}
	}
}

void sched_removeProc(sProc *p) {
	switch(p->state) {
		case ST_READY:
			sched_qDequeueProc(&readyQueue,p);
			break;
		case ST_BLOCKED:
			sched_qDequeueProc(&blockedQueue,p);
			break;
	}
}

static void sched_qInit(sQueue *q) {
	s32 i;
	sQueueNode *node;
	/* put all on the free-queue */
	node = &q->nodes[PROC_COUNT - 1];
	node->next = NULL;
	node--;
	for(i = PROC_COUNT - 2; i >= 0; i--) {
		node->next = node + 1;
		node--;
	}
	/* all free atm */
	q->free = &q->nodes[0];
	q->first = NULL;
	q->last = NULL;
}

static sProc *sched_qDequeue(sQueue *q) {
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

	return node->p;
}

static void sched_qDequeueProc(sQueue *q,sProc *p) {
	sQueueNode *n = q->first,*l = NULL;
	vassert(p != NULL,"p == NULL");

	while(n != NULL) {
		/* found it? */
		if(n->p == p) {
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

static void sched_qAppend(sQueue *q,sProc *p) {
	sQueueNode *nn,*n;
	vassert(p != NULL,"p == NULL");
	vassert(q->free != NULL,"No free slots in the queue!?");

	/* use first free node */
	nn = q->free;
	q->free = q->free->next;
	nn->p = p;
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

static void sched_qPrepend(sQueue *q,sProc *p) {
	sQueueNode *nn;
	vassert(p != NULL,"p == NULL");

	/* use first free node */
	nn = q->free;
	q->free = q->free->next;
	nn->p = p;
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
		vid_printf("\t[0x%x]: p=0x%x, pid=%d, next=0x%x\n",n,n->p,n->p->pid,n->next);
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
