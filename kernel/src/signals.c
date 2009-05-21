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
#include <proc.h>
#include <thread.h>
#include <kheap.h>
#include <signals.h>
#include <util.h>
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

/**
 * Adds a signal to the given handler and performs the corresponding action for SIG_KILL, SIG_TERM
 * and so on.
 *
 * @param h the handler (may be NULL)
 * @param pid the pid (may be INVALID_PID)
 * @param signal the signal
 * @param data the data to send
 */
static void sig_addSig(sHandler *h,tPid pid,tSig signal,u32 data);

/**
 * Finds the handler for the given thread and signal
 *
 * @param tid the thread-id
 * @param signal the signal
 * @return the handler or NULL
 */
static sHandler *sig_get(tTid tid,tSig signal);

/* to increase the speed of sig_hasSignal() store the total number of waiting signals */
u32 totalSigs = 0;
/* a linked list of handlers for each signal */
static sSLList *handler[SIG_COUNT - 1] = {NULL};

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
	vassert(sig_canHandle(signal),"Unable to handle signal %d");

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
		h = kheap_alloc(sizeof(sHandler));
		if(h == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* set / replace handler */
	/* note that we discard not yet delivered signals here.. */
	h->tid = tid;
	h->handler = func;
	h->pending = NULL;
	h->active = 0;

	if(!sll_append(handler[signal - 1],h)) {
		if(!handlerExisted)
			kheap_free(h);
		return ERR_NOT_ENOUGH_MEM;
	}

	return 0;
}

void sig_unsetHandler(tTid tid,tSig signal) {
	sHandler *h;
	vassert(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(tid,signal);
	if(h != NULL) {
		if(h->pending != NULL) {
			totalSigs -= sll_length(h->pending);
			sll_destroy(h->pending,false);
		}
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

	/* no signals at all? */
	if(totalSigs == 0)
		return false;

	/* search through all signal-lists */
	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				h = (sHandler*)n->data;
				if(h->active == 0 && sll_length(h->pending) > 0) {
					t = thread_getById(h->tid);
					/* don't deliver signals to blocked threads that wait for a msg */
					/* TODO this means that we check ALL handler-lists in EVERY interrupt
					 * until this thread is able to receive the signal... */
					if(t->signal == 0 && (t->state != ST_BLOCKED || !(t->events & EV_RECEIVED_MSG))) {
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

bool sig_addSignalFor(tPid pid,tSig signal,u32 data) {
	bool sent = false,res = false;
	sSLList *list = handler[signal - 1];
	sHandler *h;
	sThread *t;
	sSLNode *n;
	vassert(signal < SIG_COUNT,"Unable to handle signal %d");

	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			t = thread_getById(h->tid);
			if(t->proc->pid == pid) {
				sig_addSig(h,pid,signal,data);
				sent = true;
				if(!h->active && t->signal == 0)
					res = true;
			}
		}
	}
	if(!sent)
		sig_addSig(NULL,pid,signal,data);
	return res;
}

tTid sig_addSignal(tSig signal,u32 data) {
	sSLList *list;
	sHandler *h;
	sSLNode *n;
	tTid res = INVALID_TID;

	vassert(signal < SIG_COUNT,"Unable to handle signal %d");

	list = handler[signal - 1];
	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			sig_addSig(h,INVALID_PID,signal,data);
			/* remember first thread for direct notification */
			if(res == INVALID_TID && !h->active && thread_getById(h->tid)->signal == 0)
				res = h->tid;
		}
	}

	return res;
}

fSigHandler sig_startHandling(tTid tid,tSig signal) {
	sHandler *h;
	sThread *t = thread_getById(tid);
	vassert(sig_canHandle(signal),"Unable to handle signal %d");

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
	vassert(sig_canHandle(t->signal),"Unable to handle signal %d");

	h = sig_get(tid,t->signal);
	if(h != NULL) {
		t->signal = 0;
		h->active = 0;
	}
}

static void sig_addSig(sHandler *h,tPid pid,tSig signal,u32 data) {
	if(h != NULL) {
		if(h->pending == NULL) {
			h->pending = sll_create();
			if(h->pending == NULL)
				return;
		}
		if(!sll_append(h->pending,(void*)data))
			return;
		totalSigs++;
	}

	if(pid != INVALID_PID) {
		switch(signal) {
			case SIG_TERM:
			case SIG_KILL:
			case SIG_SEGFAULT:
				if(signal == SIG_KILL || h == NULL)
					proc_destroy(proc_getByPid(pid));
				else if(h != NULL) {
					/* TODO */
				}
				break;
		}
	}
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
	for(i = 0; i < SIG_COUNT - 1; i++) {
		vid_printf("\t%s:\n",sig_dbg_getName(i + 1));
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
