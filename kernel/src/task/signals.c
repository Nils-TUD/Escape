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

#include <common.h>
#include <task/proc.h>
#include <task/thread.h>
#include <task/signals.h>
#include <mem/kheap.h>
#include <util.h>
#include <kevent.h>
#include <video.h>
#include <sllist.h>
#include <errors.h>
#include <assert.h>

/* the information we need about every announced handler */
typedef struct {
	tTid tid;
	fSigHandler handler;
	u8 active;
	sSLList *pending;
} sHandler;

/* information about a signal to send; for the signal-queue */
typedef struct {
	sHandler *handler;
	tSig signal;
	u32 data;
} sSignal;

/**
 * Sends all queued signals, if possible
 */
static void sig_sendQueued(void);

/**
 * Sends queued signals, if necessary, and checks if there are signals at all
 * @return false if there are definitly no signals
 */
static bool sig_hasSigPre(void);

/**
 * Adds the given signal to the queue to pass it later to the handler
 *
 * @param h the handler
 * @param signal the signal
 * @param data the data
 */
static void sig_addToQueue(sHandler *h,tSig signal,u32 data);

/**
 * Removes all queued signals for the given handler from the queue
 *
 * @param h the handler
 */
static void sig_remFromQueue(sHandler *h);

/**
 * Adds a signal to the given handler and performs the corresponding action for SIG_KILL, SIG_TERM
 * and so on.
 *
 * @param h the handler (may be NULL)
 * @param pid the pid (may be INVALID_PID)
 * @param signal the signal
 * @param data the data to send
 * @param add whether the signal should be added to the handler
 * @return true if the signal should be handled (now or later); false it should be ignored
 */
static bool sig_addSig(sHandler *h,tPid pid,tSig signal,u32 data,bool add);

/**
 * Finds the handler for the given thread and signal
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return the handler or NULL
 */
static sHandler *sig_get(tTid tid,tSig signal);

/**
 * Kevent-handler
 */
static void sig_kWaitDone(u32 tid);

/* to increase the speed of sig_hasSignal() store the total number of waiting signals */
static u32 totalSigs = 0;
/* a linked list of handlers for each signal */
static sSLList *handler[SIG_COUNT - 1] = {NULL};
/* whether our handler is already announced in kevents */
static bool kevAdded = false;
/* whether we should look in the queue which signals we can deliver now */
static bool sendQueued = false;
/* a queue for signals that can't be delivered atm */
static sSLList *signalQueue = NULL;

void sig_init(void) {
	signalQueue = sll_create();
	if(signalQueue == NULL)
		util_panic("Unable to create signal-queue");
}

bool sig_canHandle(tSig signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

bool sig_canSend(tSig signal) {
	return signal < SIG_INTRPT_TIMER;
}

s32 sig_setHandler(tTid tid,tSig signal,fSigHandler func) {
	sHandler *h;
	bool handlerExisted = true;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);

	h = sig_get(tid,signal);
	if(h == NULL) {
		handlerExisted = false;
		/* list not existing yet? */
		if(handler[signal - 1] == NULL) {
			handler[signal - 1] = sll_create();
			if(handler[signal - 1] == NULL)
				return ERR_NOT_ENOUGH_MEM;
		}

		/* no handler present, so allocate a new one */
		h = (sHandler*)kheap_alloc(sizeof(sHandler));
		if(h == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	h->tid = tid;
	h->handler = func;
	h->pending = NULL;
	h->active = 0;

	/* don't add it twice */
	if(!handlerExisted && !sll_append(handler[signal - 1],h)) {
		kheap_free(h);
		return ERR_NOT_ENOUGH_MEM;
	}

	return 0;
}

void sig_unsetHandler(tTid tid,tSig signal) {
	sHandler *h;
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);

	h = sig_get(tid,signal);
	if(h != NULL) {
		if(h->pending != NULL) {
			totalSigs -= sll_length(h->pending);
			sll_destroy(h->pending,false);
		}
		sig_remFromQueue(h);
		sll_removeFirst(handler[signal - 1],h);
		kheap_free(h);
	}
}

void sig_removeHandlerFor(tTid tid) {
	u32 i;
	sSLNode *p,*n,*m;
	sHandler *h;
	sSLList **list;

	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			p = NULL;
			for(n = sll_begin(*list); n != NULL; ) {
				h = (sHandler*)n->data;
				if(h->tid == tid) {
					if(h->pending != NULL) {
						totalSigs -= sll_length(h->pending);
						sll_destroy(h->pending,false);
					}
					sig_remFromQueue(h);
					m = n->next;
					/* remove node */
					sll_removeNode(*list,n,p);
					kheap_free(h);
					n = m;
				}
				else {
					p = n;
					n = n->next;
				}
			}
		}
		list++;
	}
}

bool sig_hasSignal(tSig *sig,tTid *tid,u32 *data) {
	u32 i;
	sSLNode *n;
	sHandler *h;
	sSLList **list;
	sThread *t;
	if(!sig_hasSigPre())
		return false;

	/* search through all signal-lists */
	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				h = (sHandler*)n->data;
				if(h->active == 0 && sll_length(h->pending) > 0) {
					t = thread_getById(h->tid);
					/* just handle the signal if the thread doesn't already handle one */
					if(t->signal == 0) {
						*data = (u32)sll_get(h->pending,0);
						*tid = h->tid;
						*sig = i + 1;
						return true;
					}
				}
			}
		}
		list++;
	}
	return false;
}

bool sig_hasSignalFor(tTid tid) {
	u32 i;
	sSLNode *n;
	sHandler *h;
	sSLList **list;
	sThread *t;
	if(!sig_hasSigPre())
		return false;

	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				h = (sHandler*)n->data;
				if(h->tid == tid && h->active == 0 && sll_length(h->pending) > 0) {
					t = thread_getById(h->tid);
					/* just handle the signal if the thread doesn't already handle one */
					if(t->signal == 0)
						return true;
				}
			}
		}
		list++;
	}
	return false;
}

static bool sig_hasSigPre(void) {
	/* send queued signals */
	if(sendQueued) {
		sig_sendQueued();
		sendQueued = false;
	}

	/* no signals at all? */
	if(totalSigs == 0)
		return false;
	return true;
}

bool sig_addSignalFor(tPid pid,tSig signal,u32 data) {
	bool sent = false,res = false;
	sSLList *list = handler[signal - 1];
	sHandler *h;
	sThread *t;
	sSLNode *n;
	vassert(signal < SIG_COUNT,"Unable to handle signal %d",signal);

	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			t = thread_getById(h->tid);
			if(t->proc->pid == pid) {
				sent = true;
				/* if we should not ignore the signal */
				if(sig_addSig(h,pid,signal,data,t->waitsInKernel == 0)) {
					/* ensure that the thread can handle the signal atm. if not, queue it */
					if(t->waitsInKernel)
						sig_addToQueue(h,signal,data);
					else {
						if(!h->active && t->signal == 0)
							res = true;
					}
				}
			}
		}
	}
	if(!sent)
		sig_addSig(NULL,pid,signal,data,false);
	return res;
}

tTid sig_addSignal(tSig signal,u32 data) {
	sSLList *list;
	sHandler *h;
	sSLNode *n;
	sThread *t;
	tTid res = INVALID_TID;

	vassert(signal < SIG_COUNT,"Unable to handle signal %d",signal);

	list = handler[signal - 1];
	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			t = thread_getById(h->tid);
			/* if we should not ignore the signal */
			if(sig_addSig(h,INVALID_PID,signal,data,t->waitsInKernel == 0)) {
				if(t->waitsInKernel)
					sig_addToQueue(h,signal,data);
				/* remember first thread for direct notification */
				else if(res == INVALID_TID && !h->active && t->signal == 0)
					res = h->tid;
			}
		}
	}

	return res;
}

fSigHandler sig_startHandling(tTid tid,tSig signal) {
	sHandler *h;
	sThread *t = thread_getById(tid);
	vassert(sig_canHandle(signal),"Unable to handle signal %d",signal);

	h = sig_get(tid,signal);
	if(h != NULL) {
		vassert(totalSigs > 0,"We don't have any signals");
		vassert(sll_length(h->pending) > 0,"Thread %d hasn't got signal %d",tid,signal);
		sll_removeIndex(h->pending,0);
		h->active = 1;
		totalSigs--;
		/* remember the signal */
		t->signal = signal;
		return h->handler;
	}
	return NULL;
}

void sig_ackHandling(tTid tid) {
	sHandler *h;
	sThread *t = thread_getById(tid);
	vassert(t->signal != 0,"No signal handling");
	vassert(sig_canHandle(t->signal),"Unable to handle signal %d",t->signal);

	h = sig_get(tid,t->signal);
	if(h != NULL)
		h->active = 0;
	t->signal = 0;
}

static void sig_kWaitDone(u32 tid) {
	UNUSED(tid);
	sendQueued = true;
	kevAdded = false;
}

static void sig_sendQueued(void) {
	sSignal *sig;
	sThread *t;
	sSLNode *n,*p,*tn;
	p = NULL;
	for(n = sll_begin(signalQueue); n != NULL; ) {
		sig = (sSignal*)n->data;
		t = thread_getById(sig->handler->tid);
		/* thread ready now? */
		if(!t->waitsInKernel) {
			tn = n->next;
			sig_addSig(sig->handler,INVALID_PID,sig->signal,sig->data,true);
			/* remove node */
			kheap_free(sig);
			sll_removeNode(signalQueue,n,p);
			n = tn;
		}
		else {
			p = n;
			n = n->next;
		}
	}

	/* notify us again, if there are unsent signals */
	if(sll_length(signalQueue) > 0 && !kevAdded) {
		kevAdded = true;
		kev_add(KEV_KWAIT_DONE,sig_kWaitDone);
	}
}

static void sig_addToQueue(sHandler *h,tSig signal,u32 data) {
	sSignal *sig = (sSignal*)kheap_alloc(sizeof(sSignal));
	/* ignore signal, if not enough mem */
	if(sig == NULL)
		return;
	sig->handler = h;
	sig->signal = signal;
	sig->data = data;
	if(!sll_append(signalQueue,sig))
		kheap_free(sig);
	else if(!kevAdded) {
		kevAdded = true;
		kev_add(KEV_KWAIT_DONE,sig_kWaitDone);
	}
}

static void sig_remFromQueue(sHandler *h) {
	sSignal *sig;
	sSLNode *n,*p,*tn;
	p = NULL;
	for(n = sll_begin(signalQueue); n != NULL; ) {
		sig = (sSignal*)n->data;
		if(sig->handler == h) {
			tn = n->next;
			kheap_free(sig);
			sll_removeNode(signalQueue,n,p);
			n = tn;
		}
		else {
			p = n;
			n = n->next;
		}
	}
}

static bool sig_addSig(sHandler *h,tPid pid,tSig signal,u32 data,bool add) {
	if(h != NULL && h->handler != SIG_IGN && add) {
		if(h->pending == NULL) {
			h->pending = sll_create();
			if(h->pending == NULL)
				return false;
		}
		if(!sll_append(h->pending,(void*)data))
			return false;
		totalSigs++;
		return true;
	}

	if(pid != INVALID_PID) {
		switch(signal) {
			case SIG_INTRPT:
			case SIG_TERM:
			case SIG_KILL:
			case SIG_SEGFAULT:
				if(signal == SIG_KILL || h == NULL) {
					proc_terminate(proc_getByPid(pid),1,signal);
					return false;
				}
				break;
		}
	}
	return h != NULL && h->handler != SIG_IGN;
}

static sHandler *sig_get(tTid tid,tSig signal) {
	sSLList *list = handler[signal - 1];
	sHandler *h;
	sSLNode *n;
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		h = (sHandler*)n->data;
		if(h->tid == tid)
			return h;
	}
	return NULL;
}


#if DEBUGGING

u32 sig_dbg_getHandlerCount(void) {
	u32 i,c;
	sSLNode *n;
	sSLList **list;

	c = 0;
	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next)
				c++;
		}
		list++;
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
		"SIG_CHILD_DIED",
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
	u32 i;
	sSLNode *n;
	sHandler *h;
	sSLList **list;

	list = handler;
	vid_printf("Announced signal-handlers:\n");
	for(i = 1; i < SIG_COUNT - 1; i++) {
		vid_printf("\t%s:\n",sig_dbg_getName(i));
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				h = (sHandler*)n->data;
				vid_printf("\t\t(0x%08x): Tid=%d, handler=0x%x, active=%d, pending=%d\n",h,
						h->tid,h->handler,h->active,sll_length(h->pending));
			}
		}
		list++;
	}
}

#endif
