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
#include <sys/mem/cache.h>
#include <sys/util.h>
#include <sys/klock.h>
#include <sys/video.h>
#include <errors.h>
#include <assert.h>

typedef struct {
	fSignal handler;
	size_t pending;
} sSignalSlot;

typedef struct {
	sThread *thread;
	/* bitmask of pending signals */
	uint signalsPending;
	/* signal handler */
	sSignalSlot signals[SIG_COUNT];
	/* the signal that the thread is currently handling (if > 0) */
	sig_t signal;
} sSigThread;

static bool sig_isFatal(sig_t sig);
static void sig_add(sSigThread *t,sig_t sig);
static void sig_remove(sSigThread *t,sig_t sig);
static void sig_unset(sSigThread *t,sig_t sig);
static sSigThread *sig_getThread(tid_t tid,bool create);

static klock_t lock;
static size_t pendingSignals = 0;
static sSLList *sigThreads = NULL;

void sig_init(void) {
	sigThreads = sll_create();
	if(!sigThreads)
		util_panic("Unable to create signal-thread-list");
}

bool sig_canHandle(sig_t signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

bool sig_canSend(sig_t signal) {
	return signal < SIG_INTRPT_TIMER || (signal >= SIG_USR1 && signal < SIG_COUNT);
}

int sig_setHandler(tid_t tid,sig_t signal,fSignal func) {
	sSigThread *t;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	klock_aquire(&lock);
	t = sig_getThread(tid,true);
	if(!t) {
		klock_release(&lock);
		return ERR_NOT_ENOUGH_MEM;
	}
	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	t->signals[signal].handler = func;
	sig_unset(t,signal);
	klock_release(&lock);
	return 0;
}

void sig_unsetHandler(tid_t tid,sig_t signal) {
	sSigThread *t;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	klock_aquire(&lock);
	t = sig_getThread(tid,false);
	if(t) {
		t->signals[signal].handler = NULL;
		sig_unset(t,signal);
	}
	klock_release(&lock);
}

void sig_removeHandlerFor(tid_t tid) {
	size_t i;
	sSigThread *t;
	klock_aquire(&lock);
	t = sig_getThread(tid,false);
	if(t) {
		for(i = 0; i < SIG_COUNT; i++)
			sig_unset(t,i);
		sll_removeFirstWith(sigThreads,t);
		cache_free(t);
	}
	klock_release(&lock);
}

void sig_cloneHandler(tid_t parent,tid_t child) {
	size_t i;
	sSigThread *p;
	sSigThread *c;
	klock_aquire(&lock);
	p = sig_getThread(parent,false);
	if(p) {
		c = sig_getThread(child,true);
		if(c) {
			c->signalsPending = 0;
			for(i = 0; i < SIG_COUNT; i++) {
				c->signals[i].handler = p->signals[i].handler;
				c->signals[i].pending = 0;
			}
		}
	}
	klock_release(&lock);
}

bool sig_hasSignal(sig_t *sig,tid_t *tid) {
	sSLNode *n;
	klock_aquire(&lock);
	if(pendingSignals > 0) {
		for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
			sSigThread *st = (sSigThread*)n->data;
			if(st->signalsPending && st->signal == 0 && !st->thread->ignoreSignals) {
				size_t i;
				for(i = 0; i < SIG_COUNT; i++) {
					if(st->signals[i].pending) {
						*sig = i;
						*tid = st->thread->tid;
						klock_release(&lock);
						return true;
					}
				}
			}
		}
	}
	klock_release(&lock);
	return false;
}

bool sig_hasSignalFor(tid_t tid) {
	sSigThread *st;
	bool res;
	klock_aquire(&lock);
	st = sig_getThread(tid,false);
	res = st && st->signalsPending && !st->signal && !st->thread->ignoreSignals;
	klock_release(&lock);
	return res;
}

void sig_addSignalFor(pid_t pid,sig_t signal) {
	sProc *p = proc_getByPid(pid);
	sSLNode *n;
	bool sent = false;
	assert(p != NULL);
	assert(sig_canSend(signal));
	klock_aquire(&lock);
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		sSigThread *st = sig_getThread(t->tid,false);
		if(st && st->signals[signal].handler) {
			if(st->signals[signal].handler != SIG_IGN)
				sig_add(st,signal);
			sent = true;
		}
	}
	klock_release(&lock);
	/* no handler and fatal? terminate proc! */
	if(!sent && sig_isFatal(signal))
		proc_terminate(p,1,signal);
}

bool sig_addSignal(sig_t signal) {
	sSLNode *n;
	bool res = false;
	klock_aquire(&lock);
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *st = (sSigThread*)n->data;
		if(st->signals[signal].handler && st->signals[signal].handler != SIG_IGN) {
			sig_add(st,signal);
			res = true;
		}
	}
	klock_release(&lock);
	return res;
}

fSignal sig_startHandling(tid_t tid,sig_t signal) {
	sSigThread *t;
	fSignal handler;
	klock_aquire(&lock);
	t = sig_getThread(tid,false);
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	assert(t != NULL);
	assert(t->signals[signal].pending > 0);
	sig_remove(t,signal);
	t->signal = signal;
	handler = t->signals[signal].handler;
	klock_release(&lock);
	return handler;
}

sig_t sig_ackHandling(tid_t tid) {
	sig_t res;
	sSigThread *t;
	klock_aquire(&lock);
	t = sig_getThread(tid,false);
	assert(t != NULL);
	vassert(t->signal != 0,"No signal handling");
	vassert(sig_canHandle(t->signal),"Unable to handle signal %d",t->signal);
	res = t->signal;
	t->signal = 0;
	klock_release(&lock);
	return res;
}

const char *sig_dbg_getName(sig_t signal) {
	static const char *names[] = {
		"SIG_KILL",
		"SIG_TERM",
		"SIG_ILL_INSTR",
		"SIG_SEGFAULT",
		"SIG_PROC_DIED",
		"SIG_THREAD_DIED",
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
		"SIG_INTRPT_ATA2"
	};
	if(signal < SIG_COUNT)
		return names[signal];
	return "Unknown signal";
}

void sig_print(void) {
	size_t i;
	sSLNode *n;
	vid_printf("Signal handler:\n");
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *st = (sSigThread*)n->data;
		sThread *t = st->thread;
		vid_printf("\tThread %d (%d:%s)\n",t->tid,t->proc->pid,t->proc->command);
		for(i = 0; i < SIG_COUNT; i++) {
			if(st->signals[i].handler) {
				vid_printf("\t\t%s: handler=%p pending=%zu\n",
						sig_dbg_getName(i),st->signals[i].handler,st->signals[i].pending);
			}
		}
	}
}

size_t sig_dbg_getHandlerCount(void) {
	sSLNode *n;
	size_t i,c = 0;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *t = (sSigThread*)n->data;
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals[i].handler)
				c++;
		}
	}
	return c;
}

static bool sig_isFatal(sig_t sig) {
	return sig == SIG_INTRPT || sig == SIG_TERM || sig == SIG_KILL || sig == SIG_SEGFAULT ||
		sig == SIG_PIPE_CLOSED;
}

static void sig_add(sSigThread *t,sig_t sig) {
	t->signals[sig].pending++;
	t->signalsPending |= 1 << sig;
	pendingSignals++;
}

static void sig_remove(sSigThread *t,sig_t sig) {
	assert(pendingSignals > 0);
	if(--t->signals[sig].pending == 0)
		t->signalsPending &= ~(1 << sig);
	pendingSignals--;
}

static void sig_unset(sSigThread *t,sig_t sig) {
	assert(pendingSignals >= t->signals[sig].pending);
	pendingSignals -= t->signals[sig].pending;
	t->signals[sig].pending = 0;
	t->signalsPending &= ~(1 << sig);
}

static sSigThread *sig_getThread(tid_t tid,bool create) {
	sSLNode *n;
	sSigThread *st;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		st = (sSigThread*)n->data;
		if(st->thread->tid == tid)
			return st;
	}
	if(!create)
		return NULL;
	st = (sSigThread*)cache_calloc(1,sizeof(sSigThread));
	if(st == NULL)
		return NULL;
	st->thread = thread_getById(tid);
	if(!sll_append(sigThreads,st)) {
		cache_free(st);
		return NULL;
	}
	return st;
}
