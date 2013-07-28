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
#include <sys/task/proc.h>
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

klock_t Sched::lock;
Sched::Queue Sched::rdyQueues[MAX_PRIO + 1];
size_t Sched::rdyCount;
sSLList Sched::idleThreads;

void Sched::init(void) {
	size_t i;
	rdyCount = 0;
	for(i = 0; i < ARRAY_SIZE(rdyQueues); i++)
		qInit(rdyQueues + i);
	sll_init(&idleThreads,slln_allocNode,slln_freeNode);
}

void Sched::addIdleThread(Thread *t) {
	SpinLock::acquire(&lock);
	sll_append(&idleThreads,t);
	SpinLock::release(&lock);
}

Thread *Sched::perform(Thread *old,uint64_t runtime) {
	ssize_t i;
	Thread *t;
	SpinLock::acquire(&lock);
	/* give the old thread a new state */
	if(old) {
		/* TODO it would be better to keep the idle-thread if we should idle again */
		if(old->getFlags() & T_IDLE) {
			sll_append(&idleThreads,old);
			old->setState(Thread::BLOCKED);
		}
		else {
			vassert(old->getState() == Thread::RUNNING || old->getState() == Thread::ZOMBIE,"State %d",
					old->getState());
			/* we have to check for a signal here, because otherwise we might miss it */
			/* (scenario: cpu0 unblocks t1 for signal, cpu1 runs t1 and blocks itself) */
			if(old->getState() != Thread::ZOMBIE && Signals::hasSignalFor(old->getTid())) {
				/* we have to reset the newstate in this case and remove us from event */
				old->setNewState(Thread::READY);
				SpinLock::release(&lock);
				Event::removeThread(old);
				return old;
			}

			/* decrease timeslice correspondingly */
			if(runtime > old->getStats().timeslice)
				old->getStats().timeslice = 0;
			else
				old->getStats().timeslice -= runtime;

			switch(old->getNewState()) {
				case Thread::UNUSED:
				case Thread::READY:
					old->setState(Thread::READY);
					qAppend(rdyQueues + old->getPriority(),old);
					rdyCount++;
					break;
				case Thread::BLOCKED:
					old->setState(Thread::BLOCKED);
					break;
				case Thread::ZOMBIE:
					old->setState(Thread::ZOMBIE);
					break;
				default:
					Util::panic("Unexpected new state (%d)\n",old->getNewState());
					break;
			}
		}
	}

	/* get new thread */
	for(i = MAX_PRIO; i >= 0; i--) {
		t = qDequeue(rdyQueues + i);
		if(t) {
			/* if its the old thread again and we have more ready threads, don't take this one again.
			 * because we assume that Thread::switchAway() has been called for a reason. therefore, it
			 * should be better to take a thread with a lower priority than taking the same again */
			if(rdyCount > 1 && t == old) {
				qAppend(rdyQueues + i,t);
				continue;
			}
			rdyCount--;
			break;
		}
	}
	if(t == NULL) {
		/* choose an idle-thread */
		t = (Thread*)sll_removeFirst(&idleThreads);
		t->setState(Thread::RUNNING);
	}
	else {
		t->setState(Thread::RUNNING);
		t->setNewState(Thread::UNUSED);
	}

	/* if there is another thread ready, check if we have another cpu that we can start for it */
	if(rdyCount > 0)
		SMP::wakeupCPU();

	SpinLock::release(&lock);
	return t;
}

void Sched::adjustPrio(Thread *t,size_t threadCount) {
	const uint64_t threadSlice = (RUNTIME_UPDATE_INTVAL * 1000) / threadCount;
	uint64_t runtime;
	SpinLock::acquire(&lock);
	runtime = RUNTIME_UPDATE_INTVAL * 1000 - t->getStats().timeslice;
	/* if the thread has used a lot of its timeslice, lower its priority */
	if(runtime >= threadSlice * PRIO_BAD_SLICE_MULT) {
		if(t->getPriority() > 0) {
			if(t->getState() == Thread::READY)
				qDequeueThread(rdyQueues + t->getPriority(),t);
			t->setPriority(t->getPriority() - 1);
			if(t->getState() == Thread::READY)
				qAppend(rdyQueues + t->getPriority(),t);
		}
		t->prioGoodCnt = 0;
	}
	/* otherwise, raise its priority */
	else if(runtime < threadSlice / PRIO_GOOD_SLICE_DIV) {
		if(t->getPriority() < MAX_PRIO) {
			if(++t->prioGoodCnt == PRIO_FORGIVE_CNT) {
				if(t->getState() == Thread::READY)
					qDequeueThread(rdyQueues + t->getPriority(),t);
				t->setPriority(t->getPriority() + 1);
				if(t->getState() == Thread::READY)
					qAppend(rdyQueues + t->getPriority(),t);
				t->prioGoodCnt = 0;
			}
		}
	}
	t->getStats().timeslice = RUNTIME_UPDATE_INTVAL * 1000;
	SpinLock::release(&lock);
}

bool Sched::setReady(Thread *t) {
	bool res = false;
	assert(t != NULL);
	if(t->getFlags() & T_IDLE)
		return false;

	SpinLock::acquire(&lock);
	if(t->getState() == Thread::RUNNING) {
		res = t->getNewState() != Thread::READY;
		t->setNewState(Thread::READY);
	}
	else {
		if(setReadyState(t)) {
			qAppend(rdyQueues + t->getPriority(),t);
			rdyCount++;
			res = true;
		}
	}
	SpinLock::release(&lock);
	return res;
}

bool Sched::setReadyQuick(Thread *t) {
	bool res = false;
	assert(t != NULL);
	if(t->getFlags() & T_IDLE)
		return false;

	SpinLock::acquire(&lock);
	if(t->getState() == Thread::RUNNING) {
		res = t->getNewState() != Thread::READY;
		t->setNewState(Thread::READY);
	}
	else {
		if(t->getState() == Thread::READY) {
			qDequeueThread(rdyQueues + t->getPriority(),t);
			qPrepend(rdyQueues + t->getPriority(),t);
		}
		else if(setReadyState(t)) {
			qPrepend(rdyQueues + t->getPriority(),t);
			rdyCount++;
			res = true;
		}
	}
	SpinLock::release(&lock);
	return res;
}

void Sched::setBlocked(Thread *t) {
	assert(t != NULL);
	SpinLock::acquire(&lock);
	switch(t->getState()) {
		case Thread::ZOMBIE:
		case Thread::ZOMBIE_SUSP:
		case Thread::BLOCKED:
		case Thread::BLOCKED_SUSP:
			break;
		case Thread::READY_SUSP:
			t->setState(Thread::BLOCKED_SUSP);
			break;
		case Thread::RUNNING:
			t->setNewState(Thread::BLOCKED);
			break;
		case Thread::READY:
			t->setState(Thread::BLOCKED);
			qDequeueThread(rdyQueues + t->getPriority(),t);
			rdyCount--;
			break;
		default:
			vassert(false,"Invalid state for setBlocked (%d)",t->getState());
			break;
	}
	SpinLock::release(&lock);
}

void Sched::setSuspended(Thread *t,bool blocked) {
	assert(t != NULL);

	SpinLock::acquire(&lock);
	if(blocked) {
		switch(t->getState()) {
			/* already suspended, so ignore it */
			case Thread::BLOCKED_SUSP:
			case Thread::ZOMBIE_SUSP:
			case Thread::READY_SUSP:
				break;
			case Thread::BLOCKED:
				t->setState(Thread::BLOCKED_SUSP);
				break;
			case Thread::ZOMBIE:
				t->setState(Thread::ZOMBIE_SUSP);
				break;
			case Thread::READY:
				t->setState(Thread::READY_SUSP);
				qDequeueThread(rdyQueues + t->getPriority(),t);
				rdyCount--;
				break;
			default:
				vassert(false,"Thread %d has invalid state for suspending (%d)",t->getTid(),t->getState());
				break;
		}
	}
	else {
		switch(t->getState()) {
			/* not suspended, so ignore it */
			case Thread::BLOCKED:
			case Thread::ZOMBIE:
			case Thread::READY:
				break;
			case Thread::BLOCKED_SUSP:
				t->setState(Thread::BLOCKED);
				break;
			case Thread::ZOMBIE_SUSP:
				t->setState(Thread::ZOMBIE);
				break;
			case Thread::READY_SUSP:
				t->setState(Thread::READY);
				qAppend(rdyQueues + t->getPriority(),t);
				rdyCount++;
				break;
			default:
				vassert(false,"Thread %d has invalid state for resuming (%d)",t->getTid(),t->getState());
				break;
		}
	}
	SpinLock::release(&lock);
}

void Sched::removeThread(Thread *t) {
	SpinLock::acquire(&lock);
	switch(t->getState()) {
		case Thread::RUNNING:
			t->setNewState(Thread::ZOMBIE);
			SMP::killThread(t);
			SpinLock::release(&lock);
			return;
		case Thread::ZOMBIE:
		case Thread::BLOCKED:
			break;
		case Thread::READY:
			qDequeueThread(rdyQueues + t->getPriority(),t);
			rdyCount--;
			break;
		default:
			/* TODO threads can die during swap, right? */
			vassert(false,"Invalid state for removeThread (%d)",t->getState());
			break;
	}
	t->setState(Thread::ZOMBIE);
	t->setNewState(Thread::ZOMBIE);
	SpinLock::release(&lock);
}

void Sched::print(OStream &os) {
	size_t i;
	os.writef("Ready queues:\n");
	for(i = 0; i < ARRAY_SIZE(rdyQueues); i++) {
		os.writef("\t[%d]:\n",i);
		qPrint(os,rdyQueues + i);
		os.writef("\n");
	}
}

bool Sched::setReadyState(Thread *t) {
	switch(t->getState()) {
		case Thread::ZOMBIE:
		case Thread::ZOMBIE_SUSP:
		case Thread::READY:
		case Thread::READY_SUSP:
			return false;
		case Thread::BLOCKED_SUSP:
			t->setState(Thread::READY_SUSP);
			return false;
		case Thread::BLOCKED:
			break;
		default:
			vassert(false,"Invalid state for setReady (%d)",t->getState());
			break;
	}
	t->setState(Thread::READY);
	return true;
}

void Sched::qInit(Sched::Queue *q) {
	q->first = NULL;
	q->last = NULL;
}

Thread *Sched::qDequeue(Sched::Queue *q) {
	Thread *t = q->first;
	if(t == NULL)
		return NULL;

	if(t->next)
		t->next->prev = NULL;
	if(t == q->last)
		q->last = NULL;
	q->first = t->next;
	return t;
}

void Sched::qDequeueThread(Sched::Queue *q,Thread *t) {
	if(q->first == t)
		q->first = t->next;
	else
		t->prev->next = t->next;
	if(q->last == t)
		q->last = t->prev;
	else
		t->next->prev = t->prev;
}

void Sched::qAppend(Sched::Queue *q,Thread *t) {
	t->prev = q->last;
	t->next = NULL;
	if(t->prev)
		t->prev->next = t;
	else
		q->first = t;
	q->last = t;
}

void Sched::qPrepend(Sched::Queue *q,Thread *t) {
	t->prev = NULL;
	t->next = q->first;
	if(q->first)
		q->first->prev = t;
	else
		q->last = t;
	q->first = t;
}

void Sched::qPrint(OStream &os,Sched::Queue *q) {
	const Thread *t = q->first;
	while(t != NULL) {
		os.writef("\t\t%d:%d:%s\n",t->getTid(),t->getProc()->getPid(),t->getProc()->getCommand());
		t = t->next;
	}
}
