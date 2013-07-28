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
#include <sys/task/proc.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <errno.h>

/* total elapsed milliseconds */
time_t TimerBase::elapsedMsecs = 0;
time_t TimerBase::lastResched = 0;
time_t TimerBase::lastRuntimeUpdate = 0;
size_t TimerBase::timerIntrpts = 0;

klock_t TimerBase::lock;
TimerBase::Listener TimerBase::listenObjs[LISTENER_COUNT];
TimerBase::Listener *TimerBase::freeList;
TimerBase::Listener *TimerBase::listener = NULL;

void TimerBase::init(void) {
	size_t i;
	archInit();

	/* init objects */
	listenObjs->next = NULL;
	freeList = listenObjs;
	for(i = 1; i < LISTENER_COUNT; i++) {
		listenObjs[i].next = freeList;
		freeList = listenObjs + i;
	}
}

int TimerBase::sleepFor(tid_t tid,time_t msecs,bool block) {
	time_t msecDiff;
	Listener *p,*nl,*l;
	SpinLock::acquire(&lock);
	l = freeList;
	if(l == NULL) {
		SpinLock::release(&lock);
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
		Event::block(Thread::getById(tid));
	SpinLock::release(&lock);
	return 0;
}

void TimerBase::removeThread(tid_t tid) {
	Listener *l,*p;
	SpinLock::acquire(&lock);
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
	SpinLock::release(&lock);
}

bool TimerBase::intrpt(void) {
	bool res,foundThread = false;
	Listener *l,*tl;
	time_t timeInc = 1000 / FREQUENCY_DIV;

	SpinLock::acquire(&lock);
	timerIntrpts++;
	elapsedMsecs += timeInc;

	if((elapsedMsecs - lastRuntimeUpdate) >= RUNTIME_UPDATE_INTVAL) {
		Thread::updateRuntimes();
		SMP::updateRuntimes();
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
			Event::unblock(Thread::getById(l->tid));
			foundThread = true;
		}
		else
			Signals::addSignalFor(l->tid,SIG_ALARM);
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
	if(foundThread || (elapsedMsecs - lastResched) >= TIMESLICE) {
		lastResched = elapsedMsecs;
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void TimerBase::print(OStream &os) {
	time_t time;
	Listener *l;
	os.writef("Timer-Listener:\n");
	time = 0;
	for(l = listener; l != NULL; l = l->next) {
		time += l->time;
		os.writef("	diff=%u ms, rem=%u ms, thread=%d(%s), block=%d\n",l->time,time,l->tid,
				Thread::getById(l->tid)->getProc()->getCommand(),l->block);
	}
}
