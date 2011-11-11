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
#include <sys/task/smp.h>
#include <sys/task/timer.h>
#include <sys/task/event.h>
#include <sys/mem/kheap.h>
#include <sys/mem/sllnodes.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <assert.h>
#include <string.h>

/**
 * We're using round-robin here atm. That means a thread-switch puts the current thread at the end
 * of the ready-queue and makes the first in the ready-queue the new running thread.
 * Additionally we can block threads.
 *
 * Each thread has a prev- and next-pointer with which we build the ready-queue. To do so, we store
 * the beginning and end. Therefore we can dequeue the first, prepend, append and remove a thread
 * in O(1). Additionally the number of threads is limited by the kernel-heap (i.e. we don't need
 * a static storage of nodes for the linked list; we use the threads itself)
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
static void sched_qPrepend(sQueue *q,sThread *t);
static void sched_qPrint(sQueue *q);

static klock_t schedLock;
static sQueue rdyQueues[MAX_PRIO + 1];
static size_t rdyCount;
static sSLList idleThreads;

void sched_init(void) {
	size_t i;
	rdyCount = 0;
	for(i = 0; i < ARRAY_SIZE(rdyQueues); i++)
		sched_qInit(rdyQueues + i);
	sll_init(&idleThreads,slln_allocNode,slln_freeNode);
}

void sched_addIdleThread(sThread *t) {
	spinlock_aquire(&schedLock);
	sll_append(&idleThreads,t);
	spinlock_release(&schedLock);
}

sThread *sched_perform(sThread *old,uint64_t runtime) {
	ssize_t i;
	sThread *t;
	spinlock_aquire(&schedLock);
	/* give the old thread a new state */
	if(old) {
		/* TODO it would be better to keep the idle-thread if we should idle again */
		if(old->flags & T_IDLE) {
			sll_append(&idleThreads,old);
			old->state = ST_BLOCKED;
		}
		else {
			vassert(old->state == ST_RUNNING || old->state == ST_ZOMBIE,"State %d",old->state);
			/* we have to check for a signal here, because otherwise we might miss it */
			/* (scenario: cpu0 unblocks t1 for signal, cpu1 runs t1 and blocks itself) */
			if(old->state != ST_ZOMBIE && sig_hasSignalFor(old->tid)) {
				/* we have to reset the newstate in this case and remove us from event */
				old->newState = ST_READY;
				spinlock_release(&schedLock);
				ev_removeThread(old);
				return old;
			}

			/* decrease timeslice correspondingly */
			if(runtime > old->stats.timeslice)
				old->stats.timeslice = 0;
			else
				old->stats.timeslice -= runtime;

			switch(old->newState) {
				case ST_UNUSED:
				case ST_READY:
					old->state = ST_READY;
					sched_qAppend(rdyQueues + old->priority,old);
					rdyCount++;
					break;
				case ST_BLOCKED:
					old->state = ST_BLOCKED;
					break;
				case ST_ZOMBIE:
					old->state = ST_ZOMBIE;
					break;
				default:
					util_panic("Unexpected new state (%d)\n",old->newState);
					break;
			}
		}
	}

	/* get new thread */
	for(i = MAX_PRIO; i >= 0; i--) {
		t = sched_qDequeue(rdyQueues + i);
		if(t) {
			/* if its the old thread again and we have more ready threads, don't take this one again.
			 * because we assume that thread_switch() has been called for a reason. therefore, it
			 * should be better to take a thread with a lower priority than taking the same again */
			if(rdyCount > 1 && t == old) {
				sched_qAppend(rdyQueues + i,t);
				continue;
			}
			rdyCount--;
			break;
		}
	}
	if(t == NULL) {
		/* choose an idle-thread */
		t = sll_removeFirst(&idleThreads);
		t->state = ST_RUNNING;
	}
	else {
		t->state = ST_RUNNING;
		t->newState = ST_UNUSED;
	}

	/* if there is another thread ready, check if we have another cpu that we can start for it */
	if(rdyCount > 0)
		smp_wakeupCPU();

	spinlock_release(&schedLock);
	return t;
}

void sched_adjustPrio(sThread *t,size_t threadCount) {
	const uint64_t threadSlice = (RUNTIME_UPDATE_INTVAL * 1000) / threadCount;
	uint64_t runtime;
	spinlock_aquire(&schedLock);
	runtime = RUNTIME_UPDATE_INTVAL * 1000 - t->stats.timeslice;
	/* if the thread has used a lot of its timeslice, lower its priority */
	if(runtime >= threadSlice * PRIO_BAD_SLICE_MULT) {
		if(t->priority > 0) {
			if(t->state == ST_READY)
				sched_qDequeueThread(rdyQueues + t->priority,t);
			t->priority--;
			if(t->state == ST_READY)
				sched_qAppend(rdyQueues + t->priority,t);
		}
		t->prioGoodCnt = 0;
	}
	/* otherwise, raise its priority */
	else if(runtime < threadSlice / PRIO_GOOD_SLICE_DIV) {
		if(t->priority < MAX_PRIO) {
			if(++t->prioGoodCnt == PRIO_FORGIVE_CNT) {
				if(t->state == ST_READY)
					sched_qDequeueThread(rdyQueues + t->priority,t);
				t->priority++;
				if(t->state == ST_READY)
					sched_qAppend(rdyQueues + t->priority,t);
				t->prioGoodCnt = 0;
			}
		}
	}
	t->stats.timeslice = RUNTIME_UPDATE_INTVAL * 1000;
	spinlock_release(&schedLock);
}

bool sched_setReady(sThread *t) {
	bool res = false;
	assert(t != NULL);
	if(t->flags & T_IDLE)
		return false;

	spinlock_aquire(&schedLock);
	if(t->state == ST_RUNNING) {
		res = t->newState != ST_READY;
		t->newState = ST_READY;
	}
	else {
		if(sched_setReadyState(t)) {
			sched_qAppend(rdyQueues + t->priority,t);
			rdyCount++;
			res = true;
		}
	}
	spinlock_release(&schedLock);
	return res;
}

bool sched_setReadyQuick(sThread *t) {
	bool res = false;
	assert(t != NULL);
	if(t->flags & T_IDLE)
		return false;

	spinlock_aquire(&schedLock);
	if(t->state == ST_RUNNING) {
		res = t->newState != ST_READY;
		t->newState = ST_READY;
	}
	else {
		if(t->state == ST_READY) {
			sched_qDequeueThread(rdyQueues + t->priority,t);
			sched_qPrepend(rdyQueues + t->priority,t);
		}
		else if(sched_setReadyState(t)) {
			sched_qPrepend(rdyQueues + t->priority,t);
			rdyCount++;
			res = true;
		}
	}
	spinlock_release(&schedLock);
	return res;
}

static bool sched_setReadyState(sThread *t) {
	switch(t->state) {
		case ST_ZOMBIE:
		case ST_ZOMBIE_SUSP:
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
	spinlock_aquire(&schedLock);
	switch(t->state) {
		case ST_ZOMBIE:
		case ST_ZOMBIE_SUSP:
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
			sched_qDequeueThread(rdyQueues + t->priority,t);
			rdyCount--;
			break;
		default:
			vassert(false,"Invalid state for setBlocked (%d)",t->state);
			break;
	}
	spinlock_release(&schedLock);
}

void sched_setSuspended(sThread *t,bool blocked) {
	assert(t != NULL);

	spinlock_aquire(&schedLock);
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
				sched_qDequeueThread(rdyQueues + t->priority,t);
				rdyCount--;
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
				sched_qAppend(rdyQueues + t->priority,t);
				rdyCount++;
				break;
			default:
				vassert(false,"Thread %d has invalid state for resuming (%d)",t->tid,t->state);
				break;
		}
	}
	spinlock_release(&schedLock);
}

void sched_removeThread(sThread *t) {
	spinlock_aquire(&schedLock);
	switch(t->state) {
		case ST_RUNNING:
			t->newState = ST_ZOMBIE;
			smp_killThread(t);
			spinlock_release(&schedLock);
			return;
		case ST_ZOMBIE:
		case ST_BLOCKED:
			break;
		case ST_READY:
			sched_qDequeueThread(rdyQueues + t->priority,t);
			rdyCount--;
			break;
		default:
			/* TODO threads can die during swap, right? */
			vassert(false,"Invalid state for removeThread (%d)",t->state);
			break;
	}
	t->state = ST_ZOMBIE;
	t->newState = ST_ZOMBIE;
	spinlock_release(&schedLock);
}

void sched_print(void) {
	size_t i;
	vid_printf("Ready queues:\n");
	for(i = 0; i < ARRAY_SIZE(rdyQueues); i++) {
		vid_printf("\t[%d]:\n",i);
		sched_qPrint(rdyQueues + i);
		vid_printf("\n");
	}
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

static void sched_qPrepend(sQueue *q,sThread *t) {
	t->prev = NULL;
	t->next = q->first;
	if(q->first)
		q->first->prev = t;
	else
		q->last = t;
	q->first = t;
}

static void sched_qPrint(sQueue *q) {
	const sThread *t = q->first;
	while(t != NULL) {
		vid_printf("\t\t%d:%d:%s\n",t->tid,t->proc->pid,t->proc->command);
		t = t->next;
	}
}
