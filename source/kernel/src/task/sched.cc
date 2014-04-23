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

SpinLock Sched::lock;
ulong Sched::readyMask = 0;
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
	LockGuard<SpinLock> g(&lock);
	for(size_t i = 0; i < SMP::getCPUCount(); ++i) {
		if(idleThreads[i] == NULL) {
			idleThreads[i] = t;
			break;
		}
	}
}

void Sched::enqueue(Thread *t) {
	uint8_t prio = t->getPriority();
	rdyQueues[prio].append(t);
	readyMask |= 1UL << prio;
	rdyCount++;
}

void Sched::enqueueQuick(Thread *t) {
	uint8_t prio = t->getPriority();
	rdyQueues[prio].prepend(t);
	readyMask |= 1UL << prio;
	rdyCount++;
}

void Sched::dequeue(Thread *t) {
	uint8_t prio = t->getPriority();
	rdyQueues[prio].remove(t);
	if(rdyQueues[prio].length() == 0)
		readyMask &= ~(1UL << prio);
	rdyCount--;
}

Thread *Sched::perform(Thread *old,cpuid_t cpu) {
	LockGuard<SpinLock> g(&lock);
	/* give the old thread a new state */
	if(old) {
		if(old->getFlags() & T_IDLE)
			old->setState(Thread::BLOCKED);
		else {
			vassert(old->getState() == Thread::RUNNING || old->getState() == Thread::ZOMBIE,
					"State %d",old->getState());
			/* make us a zombie if we should die and have no resources anymore */
			if(old->getFlags() & T_WILL_DIE) {
				if(!old->hasResources())
					old->setNewState(Thread::ZOMBIE);
			}

			/* we have to check for a signal here, because otherwise we might miss it */
			/* (scenario: cpu0 unblocks t1 for signal, cpu1 runs t1 and blocks itself) */
			if(old->getState() != Thread::ZOMBIE && old->hasSignal()) {
				/* we have to reset the newstate in this case and remove us from event */
				old->setNewState(Thread::READY);
				old->waitstart = 0;
				removeFromEventlist(old);
				return old;
			}

			switch(old->getNewState()) {
				case Thread::READY:
					assert(old->event == 0);
					old->setState(Thread::READY);
					enqueue(old);
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
			if(rdyQueues[i].length() == 0)
				readyMask &= ~(1UL << i);
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
	return t;
}

void Sched::adjustPrio(Thread *t,uint64_t total) {
	LockGuard<SpinLock> g(&lock);
	/* if it is still blocked, add the time to the blocked time */
	if(t->waitstart > 0) {
		uint64_t now = CPU::rdtsc();
		t->stats.blocked += now - t->waitstart;
		t->waitstart = now;
	}

	/* if the thread was only blocked for a short amount of time, lower its priority */
	/* that means, if it either used the CPU quite a lot or if it wanted to do that, we consider
	 * it as evil and thus lower the priority. the reasoning behind this is that it might happen
	 * that there are more threads that want to burn the CPU than can run in our measurement-
	 * interval. in this case, it is not enough to only lower the threads that executed. because
	 * this would lead to a back and forth between these threads (group a executes -> lowered,
	 * group b executes -> lowered, group a raised, group a executed, ...). */
	/* this approach has the disadvantage that it is difficult to get raised again if a thread
	 * wanted to run for a long time but couldn't because there were others. because as long as
	 * he hasn't got its CPU time (which might be short), he will continue to try it and thus won't
	 * get a raise. */
	/* but I think this is ok because it is only a problem if we have a huge amount of threads
	 * that all want to get CPU time, so that it takes quite long for this actually "good" thread
	 * to execute. */
	/* if we consider the case where one process creates CPU intensive threads all the time, for
	 * example. with the old CPU-time-based strategy, the system was completely unusable because
	 * of the priority "bouncing". with this system, it stays usable but with the limitation above.
	 * i.e. if we're having bad luck, a "good" thread might be punished. however, in such a case
	 * the user just wants to terminate the evil guys anyway instead of continuing to use the
	 * system. */
	if(t->stats.blocked < BAD_BLOCK_TIME(total)) {
		if(t->getPriority() > 0) {
			if(t->getState() == Thread::READY)
				dequeue(t);
			t->setPriority(t->getPriority() - 1);
			if(t->getState() == Thread::READY)
				enqueue(t);
		}
		t->prioGoodCnt = 0;
	}
	/* otherwise, if it was blocked most of the time, raise the priority again */
	else if(t->stats.blocked >= GOOD_BLOCK_TIME(total)) {
		if(t->getPriority() < MAX_PRIO) {
			/* but don't do that immediately, but only if it happened multiple times */
			if(++t->prioGoodCnt == PRIO_FORGIVE_CNT) {
				if(t->getState() == Thread::READY)
					dequeue(t);
				t->setPriority(t->getPriority() + 1);
				if(t->getState() == Thread::READY)
					enqueue(t);
				t->prioGoodCnt = 0;
			}
		}
	}

	/* reset blocked time */
	t->stats.blocked = 0;
}

void Sched::wait(Thread *t,uint event,evobj_t object) {
	LockGuard<SpinLock> g(&lock);
	assert(t->event == 0);
	assert(Thread::getRunning() == t);
	t->event = event;
	t->evobject = object;
	setBlocked(t);
	if(event)
		evlists[event - 1].append(t);
}

void Sched::wakeup(uint event,evobj_t object,bool all) {
	assert(event >= 1 && event <= EV_COUNT);
	DList<Thread> *list = evlists + event - 1;
	LockGuard<SpinLock> g(&lock);
	for(auto it = list->begin(); it != list->end(); ) {
		auto old = it++;
		assert(old->event == event);
		if(old->evobject == 0 || old->evobject == object) {
			removeFromEventlist(&*old);
			setReady(&*old);
			if(!all)
				break;
		}
	}
}

void Sched::removeFromEventlist(Thread *t) {
	if(t->event) {
		/* important: remove it first from the event-list and set event to 0 */
		evlists[t->event - 1].remove(t);
		t->event = 0;
	}
}

void Sched::setReady(Thread *t) {
	if(t->getFlags() & T_IDLE)
		return;

	if(t->waitstart > 0) {
		t->stats.blocked += CPU::rdtsc() - t->waitstart;
		t->waitstart = 0;
	}

	if(t->getState() == Thread::RUNNING) {
		removeFromEventlist(t);
		t->setNewState(Thread::READY);
	}
	else if(setReadyState(t)) {
		assert(t->event == 0);
		enqueue(t);
	}
}

void Sched::setReadyQuick(Thread *t) {
	if(t->getFlags() & T_IDLE)
		return;

	if(t->waitstart > 0) {
		t->stats.blocked += CPU::rdtsc() - t->waitstart;
		t->waitstart = 0;
	}

	if(t->getState() == Thread::RUNNING) {
		removeFromEventlist(t);
		t->setNewState(Thread::READY);
	}
	else if(t->getState() == Thread::READY) {
		assert(t->event == 0);
		dequeue(t);
		enqueueQuick(t);
	}
	else if(setReadyState(t)) {
		assert(t->event == 0);
		enqueueQuick(t);
	}
}

void Sched::setBlocked(Thread *t) {
	switch(t->getState()) {
		case Thread::ZOMBIE:
		case Thread::BLOCKED:
			break;
		case Thread::RUNNING:
			t->setNewState(Thread::BLOCKED);
			break;
		case Thread::READY:
			t->setState(Thread::BLOCKED);
			dequeue(t);
			break;
		default:
			vassert(false,"Invalid state for setBlocked (%d)",t->getState());
			break;
	}
	t->waitstart = CPU::rdtsc();
}

void Sched::removeThread(Thread *t) {
	LockGuard<SpinLock> g(&lock);
	switch(t->getState()) {
		case Thread::RUNNING:
			t->setNewState(Thread::ZOMBIE);
			SMP::killThread(t);
			return;
		case Thread::ZOMBIE:
		case Thread::BLOCKED:
			removeFromEventlist(t);
			break;
		case Thread::READY:
			dequeue(t);
			break;
		default:
			/* TODO threads can die during swap, right? */
			vassert(false,"Invalid state for removeThread (%d)",t->getState());
			break;
	}
	t->setState(Thread::ZOMBIE);
	t->setNewState(Thread::ZOMBIE);
}

bool Sched::setReadyState(Thread *t) {
	switch(t->getState()) {
		case Thread::ZOMBIE:
		case Thread::READY:
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
		"MUTEX",
		"PIPE_FULL",
		"PIPE_EMPTY",
		"SWAP_JOB",
		"SWAP_WORK",
		"SWAP_FREE",
		"THREAD_DIED",
		"CHILD_DIED",
	};
	return names[event - 1];
}

void Sched::print(OStream &os,DList<Thread> *q) {
	for(auto t = q->cbegin(); t != q->cend(); ++t)
		os.writef("\t\t%d:%d:%s\n",t->getTid(),t->getProc()->getPid(),t->getProc()->getProgram());
}
