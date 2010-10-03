/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/mem/kheap.h>
#include <sys/util.h>
#include <sys/video.h>
#include <errors.h>
#include <assert.h>

typedef struct {
	fSignal handler;
	u32 pending;
} sSignalSlot;

typedef struct {
	sThread *thread;
	/* bitmask of pending signals */
	u32 signalsPending;
	/* signal handler */
	sSignalSlot signals[SIG_COUNT];
	/* the signal that the thread is currently handling (if > 0) */
	tSig signal;
} sSigThread;

static bool sig_isFatal(tSig sig);
static void sig_add(sSigThread *t,tSig sig);
static void sig_remove(sSigThread *t,tSig sig);
static void sig_unset(sSigThread *t,tSig sig);
static sSigThread *sig_getThread(tTid tid,bool create);

static u32 pendingSignals = 0;
static sSLList *sigThreads = NULL;

void sig_init(void) {
	sigThreads = sll_create();
	if(!sigThreads)
		util_panic("Unable to create signal-thread-list");
}

bool sig_canHandle(tSig signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

bool sig_canSend(tSig signal) {
	return signal < SIG_INTRPT_TIMER;
}

s32 sig_setHandler(tTid tid,tSig signal,fSignal func) {
	sSigThread *t = sig_getThread(tid,true);
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	if(!t)
		return ERR_NOT_ENOUGH_MEM;
	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	t->signals[signal].handler = func;
	sig_unset(t,signal);
	return 0;
}

void sig_unsetHandler(tTid tid,tSig signal) {
	sSigThread *t = sig_getThread(tid,false);
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	if(!t)
		return;
	t->signals[signal].handler = NULL;
	sig_unset(t,signal);
}

void sig_removeHandlerFor(tTid tid) {
	s32 i;
	sSigThread *t = sig_getThread(tid,false);
	if(!t)
		return;
	for(i = 0; i < SIG_COUNT; i++)
		sig_unset(t,i);
	sll_removeFirst(sigThreads,t);
	kheap_free(t);
}

void sig_cloneHandler(tTid parent,tTid child) {
	s32 i;
	sSigThread *p = sig_getThread(parent,false);
	sSigThread *c;
	if(!p)
		return;
	c = sig_getThread(child,true);
	c->signalsPending = 0;
	for(i = 0; i < SIG_COUNT; i++) {
		c->signals[i].handler = p->signals[i].handler;
		c->signals[i].pending = 0;
	}
}

bool sig_hasSignal(tSig *sig,tTid *tid) {
	sSLNode *n;
	if(pendingSignals == 0)
		return false;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *st = (sSigThread*)n->data;
		if(st->signalsPending && st->signal == 0 && !st->thread->ignoreSignals) {
			s32 i;
			for(i = 0; i < SIG_COUNT; i++) {
				if(st->signals[i].pending) {
					*sig = i;
					*tid = st->thread->tid;
					return true;
				}
			}
		}
	}
	return false;
}

bool sig_hasSignalFor(tTid tid) {
	sSigThread *st = sig_getThread(tid,false);
	return st && st->signalsPending && !st->signal && !st->thread->ignoreSignals;
}

void sig_addSignalFor(tPid pid,tSig signal) {
	sProc *p = proc_getByPid(pid);
	sSLNode *n;
	bool sent = false;
	assert(p != NULL);
	assert(sig_canSend(signal));
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		sSigThread *st = sig_getThread(t->tid,false);
		if(st && st->signals[signal].handler) {
			if(st->signals[signal].handler != SIG_IGN)
				sig_add(st,signal);
			sent = true;
		}
	}
	/* no handler and fatal? terminate proc! */
	if(!sent && sig_isFatal(signal))
		proc_terminate(p,1,signal);
}

void sig_addSignal(tSig signal) {
	sSLNode *n;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *st = (sSigThread*)n->data;
		if(st->signals[signal].handler && st->signals[signal].handler != SIG_IGN)
			sig_add(st,signal);
	}
}

fSignal sig_startHandling(tTid tid,tSig signal) {
	sSigThread *t = sig_getThread(tid,false);
	assert(t != NULL);
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);
	assert(t->signals[signal].pending > 0);

	sig_remove(t,signal);
	t->signal = signal;
	return t->signals[signal].handler;
}

void sig_ackHandling(tTid tid) {
	sSigThread *t = sig_getThread(tid,false);
	assert(t != NULL);
	vassert(t->signal != 0,"No signal handling");
	vassert(sig_canHandle(t->signal),"Unable to handle signal %d",t->signal);
	t->signal = 0;
}

static bool sig_isFatal(tSig sig) {
	return sig == SIG_INTRPT || sig == SIG_TERM || sig == SIG_KILL || sig == SIG_SEGFAULT ||
		sig == SIG_PIPE_CLOSED;
}

static void sig_add(sSigThread *t,tSig sig) {
	t->signals[sig].pending++;
	t->signalsPending |= 1 << sig;
	pendingSignals++;
}

static void sig_remove(sSigThread *t,tSig sig) {
	assert(pendingSignals > 0);
	if(--t->signals[sig].pending == 0)
		t->signalsPending &= ~(1 << sig);
	pendingSignals--;
}

static void sig_unset(sSigThread *t,tSig sig) {
	assert(pendingSignals >= t->signals[sig].pending);
	pendingSignals -= t->signals[sig].pending;
	t->signals[sig].pending = 0;
	t->signalsPending &= ~(1 << sig);
}

static sSigThread *sig_getThread(tTid tid,bool create) {
	sSLNode *n;
	sSigThread *st;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		st = (sSigThread*)n->data;
		if(st->thread->tid == tid)
			return st;
	}
	if(!create)
		return NULL;
	st = (sSigThread*)kheap_calloc(1,sizeof(sSigThread));
	if(st == NULL)
		return NULL;
	st->thread = thread_getById(tid);
	if(!sll_append(sigThreads,st)) {
		kheap_free(st);
		return NULL;
	}
	return st;
}


#if DEBUGGING

u32 sig_dbg_getHandlerCount(void) {
	sSLNode *n;
	u32 i,c = 0;
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *t = (sSigThread*)n->data;
		for(i = 0; i < SIG_COUNT; i++) {
			if(t->signals[i].handler)
				c++;
		}
	}
	return c;
}

const char *sig_dbg_getName(tSig signal) {
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

void sig_dbg_print(void) {
	s32 i;
	sSLNode *n;
	vid_printf("Signal handler:\n");
	for(n = sll_begin(sigThreads); n != NULL; n = n->next) {
		sSigThread *st = (sSigThread*)n->data;
		sThread *t = st->thread;
		vid_printf("\tThread %d (%d:%s)\n",t->tid,t->proc->pid,t->proc->command);
		for(i = 0; i < SIG_COUNT; i++) {
			if(st->signals[i].handler) {
				vid_printf("\t\t%s: handler=%#08x pending=%u\n",
						sig_dbg_getName(i),st->signals[i].handler,st->signals[i].pending);
			}
		}
	}
}

#endif
