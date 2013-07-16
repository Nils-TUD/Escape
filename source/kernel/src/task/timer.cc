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
#include <sys/task/sched.h>
#include <sys/task/timer.h>
#include <sys/task/event.h>
#include <sys/task/smp.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <errno.h>

#define LISTENER_COUNT		1024

/* an entry in the listener-list */
typedef struct sTimerListener {
	tid_t tid;
	/* difference to the previous listener */
	time_t time;
	/* if true, the thread is blocked during that time. otherwise it can run and will not be waked
	 * up, but gets a signal (SIG_ALARM) */
	bool block;
	struct sTimerListener *next;
} sTimerListener;

/* total elapsed milliseconds */
static time_t elapsedMsecs = 0;
static time_t lastResched = 0;
static time_t lastRuntimeUpdate = 0;
static size_t timerIntrpts = 0;

static klock_t timerLock;
static sTimerListener listenObjs[LISTENER_COUNT];
static sTimerListener *freeList;
/* processes that should be waked up to a specified time */
static sTimerListener *listener = NULL;

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
}

size_t timer_getIntrptCount(void) {
	return timerIntrpts;
}

time_t timer_getTimestamp(void) {
	return elapsedMsecs;
}

int timer_sleepFor(tid_t tid,time_t msecs,bool block) {
	time_t msecDiff;
	sTimerListener *p,*nl,*l;
	spinlock_aquire(&timerLock);
	l = freeList;
	if(l == NULL) {
		spinlock_release(&timerLock);
		return -ENOMEM;
	}

	/* remove from freelist */
	freeList = freeList->next;
	/* find place and calculate time */
	l->tid = tid;
	msecDiff = 0;
	p = NULL;
	for(nl = listener; nl != NULL; p = nl, nl = nl->next) {
		if(msecDiff + nl->time > msecs)
			break;
		msecDiff += nl->time;
	}

	l->time = msecs - msecDiff;
	l->block = block;
	/* insert entry */
	l->next = nl;
	if(p)
		p->next = l;
	else
		listener = l;

	/* now change time of next one */
	if(nl)
		nl->time -= l->time;

	/* put process to sleep */
	if(block)
		ev_block(thread_getById(tid));
	spinlock_release(&timerLock);
	return 0;
}

void timer_removeThread(tid_t tid) {
	sTimerListener *l,*p;
	spinlock_aquire(&timerLock);
	p = NULL;
	for(l = listener; l != NULL; p = l, l = l->next) {
		if(l->tid == tid) {
			/* increase time of next */
			if(l->next)
				l->next->time += l->time;
			/* remove from list */
			if(p)
				p->next = l->next;
			else
				listener = l->next;
			/* put on freelist */
			l->next = freeList;
			freeList = l;
			/* it's not possible to be in the list twice */
			break;
		}
	}
	spinlock_release(&timerLock);
}

bool timer_intrpt(void) {
	bool res,foundThread = false;
	sTimerListener *l,*tl;
	time_t timeInc = 1000 / TIMER_FREQUENCY_DIV;

	spinlock_aquire(&timerLock);
	timerIntrpts++;
	elapsedMsecs += timeInc;

	if((elapsedMsecs - lastRuntimeUpdate) >= RUNTIME_UPDATE_INTVAL) {
		thread_updateRuntimes();
		smp_updateRuntimes();
		lastRuntimeUpdate = elapsedMsecs;
	}

	/* look if there are threads to wakeup */
	for(l = listener; l != NULL; ) {
		/* stop if we have to continue waiting for this one */
		/* note that multiple listeners may have l->time = 0 */
		if(l->time > timeInc) {
			l->time -= timeInc;
			break;
		}

		timeInc -= l->time;
		/* wake up thread */
		if(l->block) {
			ev_unblock(thread_getById(l->tid));
			foundThread = true;
		}
		else
			sig_addSignalFor(l->tid,SIG_ALARM);
		/* remove from list */
		listener = l->next;
		tl = l->next;
		/* put on freelist */
		l->next = freeList;
		freeList = l;
		/* to next */
		l = tl;
	}

	/* if a process has been waked up or the time-slice is over, reschedule */
	res = false;
	if(foundThread || (elapsedMsecs - lastResched) >= PROC_TIMESLICE) {
		lastResched = elapsedMsecs;
		res = true;
	}
	spinlock_release(&timerLock);
	return res;
}

void timer_print(void) {
	time_t time;
	sTimerListener *l;
	vid_printf("Timer-Listener:\n");
	time = 0;
	for(l = listener; l != NULL; l = l->next) {
		time += l->time;
		vid_printf("	diff=%u ms, rem=%u ms, thread=%d(%s), block=%d\n",l->time,time,l->tid,
				thread_getById(l->tid)->proc->command,l->block);
	}
}
