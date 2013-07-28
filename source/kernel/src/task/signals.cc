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
#include <sys/mem/sllnodes.h>
#include <sys/mem/cache.h>
#include <sys/util.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

klock_t Signals::sigLock;
size_t Signals::pendingSignals = 0;
sSLList Signals::sigThreads;
Signals::PendingSig Signals::signals[SIGNAL_COUNT];
Signals::PendingSig *Signals::freelist;

void Signals::init(void) {
	size_t i;
	sll_init(&sigThreads,slln_allocNode,slln_freeNode);

	/* init signal-freelist */
	signals->next = NULL;
	freelist = signals;
	for(i = 1; i < SIGNAL_COUNT; i++) {
		signals[i].next = freelist;
		freelist = signals + i;
	}
}

int Signals::setHandler(tid_t tid,int signal,handler_func func,handler_func *old) {
	Data *s;
	vassert(canHandle(signal),"Unable to handle signal %d",signal);
	SpinLock::acquire(&sigLock);
	s = getThread(tid,true);
	if(!s) {
		SpinLock::release(&sigLock);
		return -ENOMEM;
	}
	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	*old = s->handler[signal];
	s->handler[signal] = func;
	removePending(s,signal);
	SpinLock::release(&sigLock);
	return 0;
}

Signals::handler_func Signals::unsetHandler(tid_t tid,int signal) {
	handler_func old = NULL;
	Data *s;
	vassert(canHandle(signal),"Unable to handle signal %d",signal);
	SpinLock::acquire(&sigLock);
	s = getThread(tid,false);
	if(s) {
		old = s->handler[signal];
		s->handler[signal] = NULL;
		removePending(s,signal);
	}
	SpinLock::release(&sigLock);
	return old;
}

void Signals::removeHandlerFor(tid_t tid) {
	Data *s;
	SpinLock::acquire(&sigLock);
	s = getThread(tid,false);
	if(s) {
		Thread *t = Thread::getById(tid);
		/* remove all pending */
		removePending(s,0);
		sll_removeFirstWith(&sigThreads,t);
		Cache::free(s);
		t->signals = NULL;
	}
	SpinLock::release(&sigLock);
}

void Signals::cloneHandler(tid_t parent,tid_t child) {
	Data *p;
	Data *c;
	SpinLock::acquire(&sigLock);
	p = getThread(parent,false);
	if(p) {
		c = getThread(child,true);
		if(c)
			memcpy(c->handler,p->handler,sizeof(p->handler));
	}
	SpinLock::release(&sigLock);
}

bool Signals::hasSignalFor(tid_t tid) {
	Data *s;
	bool res = false;
	SpinLock::acquire(&sigLock);
	s = getThread(tid,false);
	if(s && !s->currentSignal && (s->deliveredSignal || s->pending.count > 0)) {
		Thread *t = Thread::getById(tid);
		res = !t->isIgnoringSigs();
	}
	SpinLock::release(&sigLock);
	return res;
}

int Signals::checkAndStart(tid_t tid,int *sig,handler_func *handler) {
	Thread *t = Thread::getById(tid);
	Data *s;
	int res = SIG_CHECK_NO;
	SpinLock::acquire(&sigLock);
	s = t->signals;
	assert(t->isIgnoringSigs() == 0);
	if(s && s->deliveredSignal && !s->currentSignal) {
		*handler = s->handler[s->deliveredSignal];
		*sig = s->deliveredSignal;
		res = SIG_CHECK_CUR;
		s->deliveredSignal = 0;
		s->currentSignal = *sig;
	}

	if(res == SIG_CHECK_NO && pendingSignals > 0) {
		sSLNode *n;
		for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
			t = (Thread*)n->data;
			s = t->signals;
			if(!t->isIgnoringSigs() && !s->deliveredSignal && !s->currentSignal && s->pending.count > 0) {
				PendingSig *psig = s->pending.first;
				if(t->getTid() == tid) {
					*handler = s->handler[psig->sig];
					*sig = psig->sig;
					s->currentSignal = psig->sig;
					res = SIG_CHECK_CUR;
				}
				else {
					s->deliveredSignal = psig->sig;
					res = SIG_CHECK_OTHER;
				}
				/* remove signal */
				s->pending.first = psig->next;
				if(s->pending.last == psig)
					s->pending.last = NULL;
				s->pending.count--;
				pendingSignals--;
				/* put on freelist */
				psig->next = freelist;
				freelist = psig;
				break;
			}
		}
	}
	SpinLock::release(&sigLock);
	/* without locking because the scheduler calls Signals::hasSignalFor) */
	if(res == SIG_CHECK_OTHER)
		Event::unblockQuick(t);
	return res;
}

bool Signals::addSignalFor(tid_t tid,int signal) {
	Data *s;
	bool res = false;
	SpinLock::acquire(&sigLock);
	s = getThread(tid,false);
	if(s && s->handler[signal]) {
		if(s->handler[signal] != SIG_IGN)
			add(s,signal);
		res = true;
	}
	SpinLock::release(&sigLock);
	return res;
}

bool Signals::addSignal(int signal) {
	sSLNode *n;
	bool res = false;
	SpinLock::acquire(&sigLock);
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		Thread *t = (Thread*)n->data;
		if(t->signals->handler[signal] && t->signals->handler[signal] != SIG_IGN) {
			add(t->signals,signal);
			res = true;
		}
	}
	SpinLock::release(&sigLock);
	return res;
}

int Signals::ackHandling(tid_t tid) {
	int res;
	Data *s;
	SpinLock::acquire(&sigLock);
	s = getThread(tid,false);
	assert(s != NULL);
	vassert(s->currentSignal != 0,"No signal handling");
	vassert(canHandle(s->currentSignal),"Unable to handle signal %d",s->currentSignal);
	res = s->currentSignal;
	s->currentSignal = 0;
	SpinLock::release(&sigLock);
	return res;
}

const char *Signals::dbg_getName(int signal) {
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

void Signals::print(void) {
	size_t i;
	sSLNode *n;
	Video::printf("Signal handler:\n");
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		Thread *t = (Thread*)n->data;
		Video::printf("\tThread %d (%d:%s)\n",t->getTid(),t->getProc()->getPid(),t->getProc()->getCommand());
		Video::printf("\t\tpending: %zu\n",t->signals->pending.count);
		Video::printf("\t\tdeliver: %d\n",t->signals->deliveredSignal);
		Video::printf("\t\tcurrent: %d\n",t->signals->currentSignal);
		Video::printf("\t\thandler:\n");
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals->handler[i])
				Video::printf("\t\t\t%s: handler=%p\n",dbg_getName(i),t->signals->handler[i]);
		}
	}
}

size_t Signals::dbg_getHandlerCount(void) {
	sSLNode *n;
	size_t i,c = 0;
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		Thread *t = (Thread*)n->data;
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals->handler[i])
				c++;
		}
	}
	return c;
}

bool Signals::add(Data *s,int sig) {
	PendingSig *psig = freelist;
	if(psig == NULL)
		return false;

	/* remove from freelist */
	freelist = freelist->next;
	/* set properties */
	psig->sig = sig;
	psig->next = NULL;
	/* append */
	if(s->pending.last)
		s->pending.last->next = psig;
	else
		s->pending.first = psig;
	s->pending.last = psig;
	s->pending.count++;
	pendingSignals++;
	return true;
}

void Signals::removePending(Data *s,int sig) {
	PendingSig *ps,*prev;
	if(s->deliveredSignal == sig)
		s->deliveredSignal = 0;
	prev = NULL;
	for(ps = s->pending.first; ps != NULL; ) {
		if(sig == 0 || ps->sig == sig) {
			PendingSig *tps = ps->next;
			ps->next = freelist;
			freelist = ps;
			ps = tps;
			assert(pendingSignals > 0 && s->pending.count > 0);
			if(prev)
				prev->next = tps;
			else
				s->pending.first = tps;
			if(!tps)
				s->pending.last = prev;
			pendingSignals--;
			s->pending.count--;
		}
		else {
			prev = ps;
			ps = ps->next;
		}
	}
}

Signals::Data *Signals::getThread(tid_t tid,bool create) {
	Thread *t = Thread::getById(tid);
	if(t->signals == NULL) {
		if(!create)
			return NULL;
		t->signals = (Data*)Cache::calloc(1,sizeof(Data));
		if(!t->signals)
			return NULL;
		if(!sll_append(&sigThreads,t)) {
			Cache::free(t->signals);
			t->signals = NULL;
			return NULL;
		}
	}
	return t->signals;
}
