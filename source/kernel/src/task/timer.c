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

#include <sys/common.h>
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/video.h>
#include <sys/util.h>
#include <esc/sllist.h>
#include <errors.h>

#define LISTENER_COUNT		1024

/* an entry in the listener-list */
typedef struct sTimerListener {
	tTid tid;
	/* difference to the previous listener */
	tTime time;
	struct sTimerListener *next;
} sTimerListener;

/* processes that should be waked up to a specified time */
static sSLList *listener = NULL;
/* total elapsed milliseconds */
static tTime elapsedMsecs = 0;
static tTime lastResched = 0;
static size_t timerIntrpts = 0;

static sTimerListener listenObjs[LISTENER_COUNT];
static sTimerListener *freeList;

void timer_init(void) {
	size_t i;
	timer_arch_init();

	/* init objects */
	listenObjs->next = NULL;
	freeList = listenObjs;
	for(i = 1; i < LISTENER_COUNT; i++) {
		listenObjs[i].next = freeList;
		freeList = listenObjs + i;
	}

	/* init list */
	listener = sll_create();
	if(listener == NULL)
		util_panic("Not enough mem for timer-listener");
}

size_t timer_getIntrptCount(void) {
	return timerIntrpts;
}

tTime timer_getTimestamp(void) {
	return elapsedMsecs;
}

int timer_sleepFor(tTid tid,tTime msecs) {
	tTime msecDiff;
	sSLNode *n,*p;
	sTimerListener *nl,*l = freeList;
	if(l == 0)
		return ERR_NOT_ENOUGH_MEM;

	/* remove from freelist */
	freeList = freeList->next;
	/* find place and calculate time */
	l->tid = tid;
	msecDiff = 0;
	p = NULL;
	for(n = sll_begin(listener); n != NULL; p = n, n = n->next) {
		nl = (sTimerListener*)n->data;
		if(msecDiff + nl->time > msecs)
			break;
		msecDiff += nl->time;
	}

	l->time = msecs - msecDiff;
	/* store pointer to next data before inserting */
	if(n)
		nl = (sTimerListener*)n->data;
	else
		nl = NULL;
	/* insert entry */
	if(!sll_insertAfter(listener,p,l)) {
		l->next = freeList;
		freeList = l;
		return ERR_NOT_ENOUGH_MEM;
	}

	/* now change time of next one */
	if(nl)
		nl->time -= l->time;

	/* put process to sleep */
	thread_setBlocked(tid);
	return 0;
}

void timer_removeThread(tTid tid) {
	sSLNode *n,*p;
	sTimerListener *l,*nl;
	p = NULL;
	for(n = sll_begin(listener); n != NULL; p = n, n = n->next) {
		l = (sTimerListener*)n->data;
		if(l->tid == tid) {
			/* increase time of next */
			if(n->next) {
				nl = (sTimerListener*)n->next->data;
				nl->time += l->time;
			}
			sll_removeNode(listener,n,p);
			l->next = freeList;
			freeList = l;
			/* it's not possible to be in the list twice */
			break;
		}
	}
}

void timer_intrpt(void) {
	bool foundThread = false;
	sSLNode *n,*tn;
	sTimerListener *l;
	tTime timeInc = 1000 / TIMER_FREQUENCY;

	timerIntrpts++;
	elapsedMsecs += timeInc;

	/* look if there are threads to wakeup */
	for(n = sll_begin(listener); n != NULL; ) {
		l = (sTimerListener*)n->data;
		/* stop if we have to continue waiting for this one */
		/* note that multiple listeners may have l->time = 0 */
		if(l->time > timeInc) {
			l->time -= timeInc;
			break;
		}

		timeInc -= l->time;
		foundThread = true;
		tn = n->next;
		/* wake up thread */
		thread_setReady(l->tid);
		l->next = freeList;
		freeList = l;
		sll_removeNode(listener,n,NULL);
		n = tn;
	}

	/* if a process has been waked up or the time-slice is over, reschedule */
	if(foundThread || (elapsedMsecs - lastResched) >= PROC_TIMESLICE) {
		lastResched = elapsedMsecs;
		thread_switch();
	}
}

void timer_dbg_print(void) {
	tTime time;
	sSLNode *n = sll_begin(listener);
	sTimerListener *l;
	if(n != NULL) {
		vid_printf("Timer-Listener: cur=%s\n",proc_getRunning()->command);
		time = 0;
		for(; n != NULL; n = n->next) {
			l = (sTimerListener*)n->data;
			time += l->time;
			vid_printf("	diff=%d ms, rem=%d ms, tid=%d (%s)\n",l->time,time,l->tid,
					thread_getById(l->tid)->proc->command);
		}
	}
}
