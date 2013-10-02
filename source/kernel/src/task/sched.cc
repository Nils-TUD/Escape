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
#include <sys/mem/kheap.h>
#include <sys/util.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/log.h>
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
DList<Thread> Sched::rdyQueues[MAX_PRIO + 1];
DList<Thread> Sched::evlists[EV_COUNT];
size_t Sched::rdyCount;
Thread **Sched::idleThreads;

void Sched::init() {
	idleThreads = (Thread**)Cache::calloc(SMP::getCPUCount(),sizeof(Thread*));
	if(!idleThreads)
		Util::panic("Unable to allocate idle-threads array");
	rdyCount = 0;
}

void Sched::addIdleThread(Thread *t) {
	SpinLock::acquire(&lock);
	for(size_t i = 0; i < SMP::getCPUCount(); ++i) {
		if(idleThreads[i] == NULL) {
			idleThreads[i] = t;
			break;
		}
	}
	SpinLock::release(&lock);
}

Thread *Sched::perform(Thread *old,cpuid_t cpu,uint64_t runtime) {
	SpinLock::acquire(&lock);
	/* give the old thread a new state */
	if(old) {
		if(old->getFlags() & T_IDLE)
			old->setState(Thread::BLOCKED);
		else {
			vassert(old->getState() == Thread::RUNNING || old->getState() == Thread::ZOMBIE,
					"State %d",old->getState());
			/* we have to check for a signal here, because otherwise we might miss it */
			/* (scenario: cpu0 unblocks t1 for signal, cpu1 runs t1 and blocks itself) */
			if(old->getState() != Thread::ZOMBIE && Signals::hasSignalFor(old->getTid())) {
				/* we have to reset the newstate in this case and remove us from event */
				old->setNewState(Thread::READY);
				removeFromEventlist(old);
				SpinLock::release(&lock);
				return old;
			}

			/* decrease timeslice correspondingly */
			if(runtime > old->getStats().timeslice)
				old->getStats().timeslice = 0;
			else
				old->getStats().timeslice -= runtime;

			switch(old->getNewState()) {
				case Thread::READY:
					assert(old->event == 0);
					old->setState(Thread::READY);
					rdyQueues[old->getPriority()].append(old);
					rdyCount++;
					break;
				case Thread::BLOCKED:
				case Thread::ZOMBIE:
					old->setState(old->getNewState());
					break;
				default:
					Util::panic("Unexpected new state (%d)\n",old->getNewState());
					break;
			}
		}
	}

	/* get new thread */
	Thread *t;
	for(ssize_t i = MAX_PRIO; i >= 0; i--) {
		t = rdyQueues[i].removeFirst();
		if(t) {
			/* if its the old thread again and we have more ready threads, don't take this one again.
			 * because we assume that Thread::switchAway() has been called for a reason. therefore, it
			 * should be better to take a thread with a lower priority than taking the same again */
			if(rdyCount > 1 && t == old) {
				rdyQueues[i].append(t);
				continue;
			}
			rdyCount--;
			break;
		}
	}
	if(t == NULL) {
		/* choose an idle-thread */
		t = idleThreads[cpu];
		t->setState(Thread::RUNNING);
	}
	else {
		t->setState(Thread::RUNNING);
		t->setNewState(Thread::READY);
	}

	/* if there is another thread ready, check if we have another cpu that we can start for it */
	if(rdyCount > 0)
		SMP::wakeupCPU();

	SpinLock::release(&lock);
	return t;
}

void Sched::adjustPrio(Thread *t,size_t threadCount) {
	const uint64_t interval = RUNTIME_UPDATE_INTVAL * (CPU::getSpeed() / 1000);
	const uint64_t threadSlice = interval / threadCount;
	SpinLock::acquire(&lock);
	uint64_t runtime = interval - t->getStats().timeslice;
	/* if the thread has used a lot of its timeslice, lower its priority */
	if(runtime >= threadSlice * PRIO_BAD_SLICE_MULT) {
		if(t->getPriority() > 0) {
			if(t->getState() == Thread::READY)
				rdyQueues[t->getPriority()].remove(t);
			t->setPriority(t->getPriority() - 1);
			if(t->getState() == Thread::READY)
				rdyQueues[t->getPriority()].append(t);
		}
		t->prioGoodCnt = 0;
	}
	/* otherwise, raise its priority */
	else if(runtime < threadSlice / PRIO_GOOD_SLICE_DIV) {
		if(t->getPriority() < MAX_PRIO) {
			if(++t->prioGoodCnt == PRIO_FORGIVE_CNT) {
				if(t->getState() == Thread::READY)
					rdyQueues[t->getPriority()].remove(t);
				t->setPriority(t->getPriority() + 1);
				if(t->getState() == Thread::READY)
					rdyQueues[t->getPriority()].append(t);
				t->prioGoodCnt = 0;
			}
		}
	}
	t->getStats().timeslice = interval;
	SpinLock::release(&lock);
}

void Sched::wait(Thread *t,uint event,evobj_t object) {
	SpinLock::acquire(&lock);
	assert(t->event == 0);
	assert(Thread::getRunning() == t);
	t->event = event;
	t->evobject = object;
	setBlocked(t);
	if(event)
		evlists[event - 1].append(t);
	SpinLock::release(&lock);
}

void Sched::wakeup(uint event,evobj_t object) {
	assert(event >= 1 && event <= EV_COUNT);
	DList<Thread> *list = evlists + event - 1;
	SpinLock::acquire(&lock);
	for(auto it = list->begin(); it != list->end(); ) {
		auto old = it++;
		assert(old->event == event);
		if(old->evobject == 0 || old->evobject == object) {
			/* important: remove it first from the event-list and set event to 0 */
			list->remove(&*old);
			old->event = 0;
			setReady(&*old);
		}
	}
	SpinLock::release(&lock);
}

bool Sched::wakeupThread(Thread *t,uint event) {
	assert(event >= 1 && event <= EV_COUNT);
	bool res = false;
	SpinLock::acquire(&lock);
	if(t->event == event) {
		/* important: remove it first from the event-list and set event to 0 */
		evlists[event - 1].remove(t);
		t->event = 0;
		setReady(t);
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void Sched::setReady(Thread *t) {
	if(t->getFlags() & T_IDLE)
		return;

	if(t->getState() == Thread::RUNNING) {
		removeFromEventlist(t);
		t->setNewState(Thread::READY);
	}
	else if(setReadyState(t)) {
		assert(t->event == 0);
		rdyQueues[t->getPriority()].append(t);
		rdyCount++;
	}
}

void Sched::setReadyQuick(Thread *t) {
	if(t->getFlags() & T_IDLE)
		return;

	if(t->getState() == Thread::RUNNING) {
		removeFromEventlist(t);
		t->setNewState(Thread::READY);
	}
	else if(t->getState() == Thread::READY) {
		assert(t->event == 0);
		rdyQueues[t->getPriority()].remove(t);
		rdyQueues[t->getPriority()].prepend(t);
	}
	else if(setReadyState(t)) {
		assert(t->event == 0);
		rdyQueues[t->getPriority()].prepend(t);
		rdyCount++;
	}
}

void Sched::removeFromEventlist(Thread *t) {
	if(t->event) {
		evlists[t->event - 1].remove(t);
		t->event = 0;
	}
}

void Sched::setBlocked(Thread *t) {
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
			rdyQueues[t->getPriority()].remove(t);
			rdyCount--;
			break;
		default:
			vassert(false,"Invalid state for setBlocked (%d)",t->getState());
			break;
	}
}

void Sched::setSuspended(Thread *t,bool blocked) {
	/* TODO what if we're waiting for an event? */
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
				rdyQueues[t->getPriority()].remove(t);
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
				rdyQueues[t->getPriority()].append(t);
				rdyCount++;
				break;
			default:
				vassert(false,"Thread %d has invalid state for resuming (%d)",t->getTid(),t->getState());
				break;
		}
	}
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
			removeFromEventlist(t);
			break;
		case Thread::READY:
			rdyQueues[t->getPriority()].remove(t);
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
			removeFromEventlist(t);
			break;
		default:
			vassert(false,"Invalid state for setReady (%d)",t->getState());
			break;
	}
	t->setState(Thread::READY);
	return true;
}

void Sched::print(OStream &os) {
	os.writef("Ready queues:\n");
	for(size_t i = 0; i < ARRAY_SIZE(rdyQueues); i++) {
		os.writef("\t[%d]:\n",i);
		print(os,rdyQueues + i);
		os.writef("\n");
	}
}

void Sched::printEventLists(OStream &os) {
	os.writef("Eventlists:\n");
	for(size_t e = 0; e < EV_COUNT; e++) {
		DList<Thread> *list = evlists + e;
		os.writef("\t%s:\n",getEventName(e + 1));
		for(auto t = list->cbegin(); t != list->cend(); ++t) {
			os.writef("\t\tthread=%d (%d:%s), object=%x",
					t->getTid(),t->getProc()->getPid(),t->getProc()->getProgram(),t->evobject);
			inode_t nodeNo = ((VFSNode*)t->evobject)->getNo();
			if(VFSNode::isValid(nodeNo))
				os.writef("(%s)",((VFSNode*)t->evobject)->getPath());
			os.writef("\n");
		}
	}
}

const char *Sched::getEventName(uint event) {
	static const char *names[] = {
		"CLIENT",
		"RECEIVED_MSG",
		"DATA_READABLE",
		"MUTEX",
		"PIPE_FULL",
		"PIPE_EMPTY",
		"UNLOCK_SH",
		"UNLOCK_EX",
		"REQ_FREE",
		"USER1",
		"USER2",
		"SWAP_JOB",
		"SWAP_WORK",
		"SWAP_FREE",
		"VMM_DONE",
		"THREAD_DIED",
		"CHILD_DIED",
		"TERMINATION",
	};
	return names[event - 1];
}

void Sched::print(OStream &os,DList<Thread> *q) {
	for(auto t = q->cbegin(); t != q->cend(); ++t)
		os.writef("\t\t%d:%d:%s\n",t->getTid(),t->getProc()->getPid(),t->getProc()->getProgram());
}
