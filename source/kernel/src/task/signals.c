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

#define SIGNAL_COUNT	2048
#define MAX_SIG_RECV	16

static bool sig_add(sSignals *s,sig_t sig);
static void sig_removePending(sSignals *s,sig_t sig);
static sSignals *sig_getThread(tid_t tid,bool create);

static klock_t sigLock;
static size_t pendingSignals = 0;
static sSLList sigThreads;
static sPendingSig signals[SIGNAL_COUNT];
static sPendingSig *freelist;

void sig_init(void) {
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

bool sig_canHandle(sig_t signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

bool sig_canSend(sig_t signal) {
	return signal < SIG_INTRPT_TIMER || (signal >= SIG_USR1 && signal < SIG_COUNT);
}

bool sig_isFatal(sig_t sig) {
	return sig == SIG_INTRPT || sig == SIG_TERM || sig == SIG_KILL || sig == SIG_SEGFAULT ||
		sig == SIG_PIPE_CLOSED;
}

int sig_setHandler(tid_t tid,sig_t signal,fSignal func) {
	sSignals *s;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,true);
	if(!s) {
		spinlock_release(&sigLock);
		return -ENOMEM;
	}
	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	s->handler[signal] = func;
	sig_removePending(s,signal);
	spinlock_release(&sigLock);
	return 0;
}

void sig_unsetHandler(tid_t tid,sig_t signal) {
	sSignals *s;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,false);
	if(s) {
		s->handler[signal] = NULL;
		sig_removePending(s,signal);
	}
	spinlock_release(&sigLock);
}

void sig_removeHandlerFor(tid_t tid) {
	sSignals *s;
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,false);
	if(s) {
		sThread *t = thread_getById(tid);
		/* remove all pending */
		sig_removePending(s,0);
		sll_removeFirstWith(&sigThreads,t);
		cache_free(s);
		t->signals = NULL;
	}
	spinlock_release(&sigLock);
}

void sig_cloneHandler(tid_t parent,tid_t child) {
	sSignals *p;
	sSignals *c;
	spinlock_aquire(&sigLock);
	p = sig_getThread(parent,false);
	if(p) {
		c = sig_getThread(child,true);
		if(c)
			memcpy(c->handler,p->handler,sizeof(p->handler));
	}
	spinlock_release(&sigLock);
}

bool sig_hasSignalFor(tid_t tid) {
	sSignals *s;
	bool res = false;
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,false);
	if(s && !s->currentSignal && (s->deliveredSignal || s->pending.count > 0)) {
		sThread *t = thread_getById(tid);
		res = !t->ignoreSignals;
	}
	spinlock_release(&sigLock);
	return res;
}

int sig_checkAndStart(tid_t tid,sig_t *sig,fSignal *handler) {
	sThread *t = thread_getById(tid);
	sSignals *s;
	int res = SIG_CHECK_NO;
	spinlock_aquire(&sigLock);
	s = t->signals;
	assert(t->ignoreSignals == 0);
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
			t = (sThread*)n->data;
			s = t->signals;
			if(!t->ignoreSignals && !s->deliveredSignal && !s->currentSignal && s->pending.count > 0) {
				sPendingSig *psig = s->pending.first;
				if(t->tid == tid) {
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
	spinlock_release(&sigLock);
	/* without locking because the scheduler calls sig_hasSignalFor() */
	if(res == SIG_CHECK_OTHER)
		ev_unblockQuick(t);
	return res;
}

bool sig_addSignalFor(tid_t tid,sig_t signal) {
	sSignals *s;
	bool res = false;
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,false);
	if(s && s->handler[signal]) {
		if(s->handler[signal] != SIG_IGN)
			sig_add(s,signal);
		res = true;
	}
	spinlock_release(&sigLock);
	return res;
}

bool sig_addSignal(sig_t signal) {
	sSLNode *n;
	bool res = false;
	spinlock_aquire(&sigLock);
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		if(t->signals->handler[signal] && t->signals->handler[signal] != SIG_IGN) {
			sig_add(t->signals,signal);
			res = true;
		}
	}
	spinlock_release(&sigLock);
	return res;
}

sig_t sig_ackHandling(tid_t tid) {
	sig_t res;
	sSignals *s;
	spinlock_aquire(&sigLock);
	s = sig_getThread(tid,false);
	assert(s != NULL);
	vassert(s->currentSignal != 0,"No signal handling");
	vassert(sig_canHandle(s->currentSignal),"Unable to handle signal %d",s->currentSignal);
	res = s->currentSignal;
	s->currentSignal = 0;
	spinlock_release(&sigLock);
	return res;
}

const char *sig_dbg_getName(sig_t signal) {
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

void sig_print(void) {
	size_t i;
	sSLNode *n;
	vid_printf("Signal handler:\n");
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		vid_printf("\tThread %d (%d:%s)\n",t->tid,t->proc->pid,t->proc->command);
		vid_printf("\t\tpending: %zu\n",t->signals->pending.count);
		vid_printf("\t\tdeliver: %d\n",t->signals->deliveredSignal);
		vid_printf("\t\tcurrent: %d\n",t->signals->currentSignal);
		vid_printf("\t\thandler:\n");
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals->handler[i])
				vid_printf("\t\t\t%s: handler=%p\n",sig_dbg_getName(i),t->signals->handler[i]);
		}
	}
}

size_t sig_dbg_getHandlerCount(void) {
	sSLNode *n;
	size_t i,c = 0;
	for(n = sll_begin(&sigThreads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals->handler[i])
				c++;
		}
	}
	return c;
}

static bool sig_add(sSignals *s,sig_t sig) {
	sPendingSig *psig = freelist;
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

static void sig_removePending(sSignals *s,sig_t sig) {
	sPendingSig *ps,*prev;
	if(s->deliveredSignal == sig)
		s->deliveredSignal = 0;
	prev = NULL;
	for(ps = s->pending.first; ps != NULL; ) {
		if(sig == 0 || ps->sig == sig) {
			sPendingSig *tps = ps->next;
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

static sSignals *sig_getThread(tid_t tid,bool create) {
	sThread *t = thread_getById(tid);
	if(t->signals == NULL) {
		if(!create)
			return NULL;
		t->signals = (sSignals*)cache_calloc(1,sizeof(sSignals));
		if(!t->signals)
			return NULL;
		if(!sll_append(&sigThreads,t)) {
			cache_free(t->signals);
			t->signals = NULL;
			return NULL;
		}
	}
	return t->signals;
}
