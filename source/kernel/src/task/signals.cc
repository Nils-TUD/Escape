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
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/col/dlist.h>
#include <sys/mem/cache.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

klock_t Signals::lock;
klock_t Signals::listLock;
Signals::PendingSig Signals::signals[SIGNAL_COUNT];
Signals::PendingSig *Signals::freelist;
static DList<Thread::ListItem> sigThreads;

void Signals::init() {
	/* init signal-freelist */
	signals->next = NULL;
	freelist = signals;
	for(size_t i = 1; i < SIGNAL_COUNT; i++) {
		signals[i].next = freelist;
		freelist = signals + i;
	}
}

int Signals::setHandler(tid_t tid,int signal,handler_func func,handler_func *old) {
	vassert(canHandle(signal),"Unable to handle signal %d",signal);
	assert(tid == Thread::getRunning()->getTid());
	Thread *t = getThread(tid);
	if(!t)
		return -ENOMEM;
	SpinLock::acquire(&lock);
	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	*old = t->sigHandler[signal];
	t->sigHandler[signal] = func;
	removePending(t,signal);
	SpinLock::release(&lock);
	return 0;
}

Signals::handler_func Signals::unsetHandler(tid_t tid,int signal) {
	handler_func old = NULL;
	vassert(canHandle(signal),"Unable to handle signal %d",signal);
	SpinLock::acquire(&lock);
	Thread *t = Thread::getById(tid);
	if(t->sigHandler) {
		old = t->sigHandler[signal];
		t->sigHandler[signal] = NULL;
		removePending(t,signal);
	}
	SpinLock::release(&lock);
	return old;
}

void Signals::removeHandlerFor(tid_t tid) {
	SpinLock::acquire(&listLock);
	SpinLock::acquire(&lock);
	Thread *t = Thread::getById(tid);
	if(t->sigHandler) {
		/* remove all pending */
		removePending(t,0);
		sigThreads.remove(&t->signalListItem);
		Cache::free(t->sigHandler);
		t->sigHandler = NULL;
	}
	SpinLock::release(&lock);
	SpinLock::release(&listLock);
}

void Signals::cloneHandler(tid_t parent,tid_t child) {
	Thread *p = Thread::getById(parent);
	if(p->sigHandler) {
		Thread *c = getThread(child);
		if(c) {
			SpinLock::acquire(&lock);
			memcpy(c->sigHandler,p->sigHandler,SIG_COUNT * sizeof(handler_func));
			SpinLock::release(&lock);
		}
	}
}

bool Signals::hasSignalFor(tid_t tid) {
	bool res = false;
	SpinLock::acquire(&lock);
	Thread *t = Thread::getById(tid);
	if(t->sigHandler && !t->currentSignal && t->pending.count > 0)
		res = !t->isIgnoringSigs();
	SpinLock::release(&lock);
	return res;
}

bool Signals::checkAndStart(tid_t tid,int *sig,handler_func *handler) {
	bool res = false;
	Thread *t = Thread::getById(tid);
	SpinLock::acquire(&lock);
	assert(t->isIgnoringSigs() == 0 && t->sigHandler);
	if(!t->currentSignal) {
		PendingSig *psig = t->pending.first;
		*handler = t->sigHandler[psig->sig];
		*sig = psig->sig;
		t->currentSignal = psig->sig;
		/* remove signal */
		t->pending.first = psig->next;
		if(t->pending.last == psig)
			t->pending.last = NULL;
		t->pending.count--;
		/* put on freelist */
		psig->next = freelist;
		freelist = psig;
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

bool Signals::addSignalFor(tid_t tid,int signal) {
	bool res = false;
	SpinLock::acquire(&lock);
	Thread *t = Thread::getById(tid);
	if(t->sigHandler && t->sigHandler[signal]) {
		if(t->sigHandler[signal] != SIG_IGN)
			add(t,signal);
		res = true;
	}
	SpinLock::release(&lock);
	/* without locking because the scheduler calls Signals::hasSignalFor) */
	if(res)
		Event::unblockQuick(t);
	return res;
}

bool Signals::addSignal(int signal) {
	bool res = false;
	SpinLock::acquire(&listLock);
	for(auto it = sigThreads.begin(); it != sigThreads.end(); ++it) {
		SpinLock::acquire(&lock);
		bool added = false;
		if(it->thread->sigHandler[signal] && it->thread->sigHandler[signal] != SIG_IGN) {
			add(it->thread,signal);
			added = true;
			res = true;
		}
		SpinLock::release(&lock);
		/* without locking because the scheduler calls Signals::hasSignalFor).
		 * the listlock is ok because we don't grab it in hasSignalFor. */
		if(added)
			Event::unblockQuick(it->thread);
	}
	SpinLock::release(&listLock);
	return res;
}

int Signals::ackHandling(tid_t tid) {
	SpinLock::acquire(&lock);
	Thread *t = Thread::getById(tid);
	assert(t != NULL);
	vassert(t->currentSignal != 0,"No signal handling");
	vassert(canHandle(t->currentSignal),"Unable to handle signal %d",t->currentSignal);
	int res = t->currentSignal;
	t->currentSignal = 0;
	SpinLock::release(&lock);
	return res;
}

const char *Signals::getName(int signal) {
	static const char *names[] = {
		"SIG_KILL",
		"SIG_TERM",
		"SIG_ILL_INSTR",
		"SIG_SEGFAULT",
		"SIG_PIPE_CLOSED",
		"SIG_CHILD_TERM",
		"SIG_INTRPT",
		"SIG_INTRPT_TIMER",
		"SIG_INTRPT_KB",
		"SIG_INTRPT_COM1",
		"SIG_INTRPT_COM2",
		"SIG_INTRPT_FLOPPY",
		"SIG_INTRPT_CMOS",
		"SIG_INTRPT_ATA1",
		"SIG_INTRPT_ATA2",
		"SIG_INTRPT_MOUSE",
		"SIG_ALARM",
		"SIG_USR1",
		"SIG_USR2"
	};
	if(signal < SIG_COUNT)
		return names[signal];
	return "Unknown signal";
}

void Signals::print(OStream &os) {
	os.writef("Signal handler:\n");
	SpinLock::acquire(&listLock);
	for(auto it = sigThreads.cbegin(); it != sigThreads.cend(); ++it) {
		const Thread *t = it->thread;
		os.writef("\tThread %d (%d:%s)\n",t->getTid(),t->getProc()->getPid(),t->getProc()->getCommand());
		os.writef("\t\tpending: %zu\n",t->pending.count);
		os.writef("\t\tcurrent: %d\n",t->currentSignal);
		os.writef("\t\thandler:\n");
		for(size_t i = 0; i < SIG_COUNT; i++) {
			if(t->sigHandler[i])
				os.writef("\t\t\t%s: handler=%p\n",getName(i),t->sigHandler[i]);
		}
	}
	SpinLock::release(&listLock);
}

size_t Signals::getHandlerCount() {
	size_t c = 0;
	SpinLock::acquire(&listLock);
	for(auto it = sigThreads.cbegin(); it != sigThreads.cend(); ++it) {
		for(size_t i = 0; i < SIG_COUNT; i++) {
			if(it->thread->sigHandler[i])
				c++;
		}
	}
	SpinLock::release(&listLock);
	return c;
}

bool Signals::add(Thread *t,int sig) {
	PendingSig *psig = freelist;
	if(psig == NULL)
		return false;

	/* remove from freelist */
	freelist = freelist->next;
	/* set properties */
	psig->sig = sig;
	psig->next = NULL;
	/* append */
	if(t->pending.last)
		t->pending.last->next = psig;
	else
		t->pending.first = psig;
	t->pending.last = psig;
	t->pending.count++;
	return true;
}

void Signals::removePending(Thread *t,int sig) {
	PendingSig *prev = NULL;
	for(PendingSig *ps = t->pending.first; ps != NULL; ) {
		if(sig == 0 || ps->sig == sig) {
			PendingSig *tps = ps->next;
			ps->next = freelist;
			freelist = ps;
			ps = tps;
			assert(t->pending.count > 0);
			if(prev)
				prev->next = tps;
			else
				t->pending.first = tps;
			if(!tps)
				t->pending.last = prev;
			t->pending.count--;
		}
		else {
			prev = ps;
			ps = ps->next;
		}
	}
}

Thread *Signals::getThread(tid_t tid) {
	Thread *t = Thread::getById(tid);
	if(t->sigHandler == NULL) {
		t->sigHandler = (handler_func*)Cache::calloc(SIG_COUNT,sizeof(handler_func));
		if(!t->sigHandler)
			return NULL;
		SpinLock::acquire(&listLock);
		sigThreads.append(&t->signalListItem);
		SpinLock::release(&listLock);
	}
	return t;
}
