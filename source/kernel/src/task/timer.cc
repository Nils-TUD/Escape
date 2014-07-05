/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/task/smp.h>
#include <sys/task/proc.h>
#include <sys/video.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <errno.h>

/* total elapsed milliseconds */
TimerBase::PerCPU *TimerBase::perCPU = NULL;
time_t TimerBase::lastRuntimeUpdate = 0;

SpinLock TimerBase::lock;
TimerBase::Listener TimerBase::listenObjs[LISTENER_COUNT];
TimerBase::Listener *TimerBase::freeList;
TimerBase::Listener *TimerBase::listener = NULL;

void TimerBase::init() {
	archInit();

	perCPU = (PerCPU*)Cache::calloc(SMP::getCPUCount(),sizeof(PerCPU));
	if(!perCPU)
		Util::panic("Unable to create per-cpu-array");

	/* init objects */
	listenObjs->next = NULL;
	freeList = listenObjs;
	for(size_t i = 1; i < LISTENER_COUNT; i++) {
		listenObjs[i].next = freeList;
		freeList = listenObjs + i;
	}
}

int TimerBase::sleepFor(tid_t tid,time_t msecs,bool block) {
	LockGuard<SpinLock> g(&lock);
	Listener *l = freeList;
	if(l == NULL)
		return -ENOMEM;

	/* remove from freelist */
	freeList = freeList->next;
	/* find place and calculate time */
	l->tid = tid;
	time_t msecDiff = 0;
	Listener *p = NULL;
	Listener *nl;
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
		Thread::getById(tid)->block();
	return 0;
}

void TimerBase::removeThread(tid_t tid) {
	LockGuard<SpinLock> g(&lock);
	Listener *p = NULL;
	for(Listener *l = listener; l != NULL; p = l, l = l->next) {
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
}

bool TimerBase::intrpt() {
	bool res,foundThread = false;
	time_t timeInc = 1000 / FREQUENCY_DIV;
	cpuid_t cpu = Thread::getRunning()->getCPU();

	perCPU[cpu].timerIntrpts++;
	perCPU[cpu].elapsedMsecs += timeInc;

	if(cpu == 0) {
		LockGuard<SpinLock> g(&lock);
		if((perCPU[cpu].elapsedMsecs - lastRuntimeUpdate) >= RUNTIME_UPDATE_INTVAL) {
			Thread::updateRuntimes();
			SMP::updateRuntimes();
			lastRuntimeUpdate = perCPU[cpu].elapsedMsecs;
		}

		/* look if there are threads to wakeup */
		for(Listener *l = listener; l != NULL; ) {
			/* stop if we have to continue waiting for this one */
			/* note that multiple listeners may have l->time = 0 */
			if(l->time > timeInc) {
				l->time -= timeInc;
				break;
			}

			timeInc -= l->time;
			/* wake up thread */
			Thread *t = Thread::getById(l->tid);
			if(l->block) {
				t->unblock();
				foundThread = true;
			}
			else
				Signals::addSignalFor(t,SIG_ALARM);
			/* remove from list */
			listener = l->next;
			Listener *tl = l->next;
			/* put on freelist */
			l->next = freeList;
			freeList = l;
			/* to next */
			l = tl;
		}
	}

	/* if a process has been waked up or the time-slice is over, reschedule */
	res = false;
	if(foundThread || (perCPU[cpu].elapsedMsecs - perCPU[cpu].lastResched) >= TIMESLICE) {
		perCPU[cpu].lastResched = perCPU[cpu].elapsedMsecs;
		res = true;
	}
	return res;
}

void TimerBase::print(OStream &os) {
	os.writef("Timer-Listener:\n");
	time_t time = 0;
	for(Listener *l = listener; l != NULL; l = l->next) {
		time += l->time;
		os.writef("	diff=%u ms, rem=%u ms, thread=%d(%s), block=%d\n",l->time,time,l->tid,
				Thread::getById(l->tid)->getProc()->getProgram(),l->block);
	}
}
