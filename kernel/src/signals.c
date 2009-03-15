/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/kheap.h"
#include "../h/signals.h"
#include "../h/util.h"
#include <sllist.h>

/* the information we need about every announced handler */
typedef struct {
	tPid pid;
	fSigHandler handler;
	u8 active;
	u8 sigCount;
} sHandler;

/**
 * Finds the handler for the given process and signal
 *
 * @param pid the process-id
 * @param signal the signal
 * @return the handler or NULL
 */
static sHandler *sig_get(tPid pid,u8 signal);

/* to increase the speed of sig_hasSignal() store the total number of waiting signals */
u32 totalSigs = 0;
/* a linked list of handlers for each signal */
static sSLList *handler[SIG_COUNT - 1];

void sig_init(void) {
	u32 i;
	/* skip SIG_KILL */
	for(i = 0; i < SIG_COUNT - 1; i++) {
		handler[i] = sll_create();
		if(handler[i] == NULL)
			panic("Not enough mem for signal-handling");
	}
}

bool sig_canHandle(u8 signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

s32 sig_setHandler(tPid pid,u8 signal,fSigHandler func) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	/* TODO keep that? would be better to replace the handler, right? */
	if(sig_get(pid,signal) != NULL)
		return ERR_SIG_HANDLER_EXISTS;

	h = kheap_alloc(sizeof(sHandler));
	if(h == NULL)
		return ERR_NOT_ENOUGH_MEM;

	h->pid = pid;
	h->handler = func;
	h->sigCount = 0;
	h->active = 0;
	if(!sll_append(handler[signal - 1],h)) {
		kheap_free(h);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

void sig_unsetHandler(tPid pid,u8 signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		sll_removeFirst(handler[signal - 1],h);
		kheap_free(h);
	}
}

bool sig_hasSignal(u8 *sig,tPid *pid) {
	u32 i;
	sSLNode *n;
	sHandler *h;
	sSLList *list;

	/* no signals at all? */
	if(totalSigs == 0)
		return false;

	/* search through all signal-lists */
	list = handler[0];
	for(i = 0; i < SIG_COUNT - 1; i++) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			if(h->sigCount > 0) {
				*pid = h->pid;
				*sig = i + 1;
				return true;
			}
		}
		list++;
	}
	return false;
}

bool sig_addSignalFor(tPid pid,u8 signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		h->sigCount++;
		return !h->active;
	}
	return false;
}

tPid sig_addSignal(u8 signal) {
	sSLList *list;
	sHandler *h;
	sSLNode *n;
	tPid res = INVALID_PID;

	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	list = handler[signal - 1];
	for(n = sll_begin(list); n != NULL; n = n->next) {
		h = (sHandler*)n->data;
		h->sigCount++;
		totalSigs++;
		/* remember first proc for direct notification */
		if(!h->active && res == INVALID_PID)
			res = h->pid;
	}

	return res;
}

void sig_startHandling(tPid pid,u8 signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		ASSERT(totalSigs > 0,"We don't have any signals");
		ASSERT(h->sigCount > 0,"Process %d hasn't got signal %d",pid,signal);
		h->sigCount--;
		h->active = 1;
		totalSigs--;
	}
}

void sig_ackHandling(tPid pid,u8 signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL)
		h->active = 0;
}

static sHandler *sig_get(tPid pid,u8 signal) {
	sSLList *list = handler[signal - 1];
	sHandler *h;
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		h = (sHandler*)n->data;
		if(h->pid == pid)
			return h;
	}
	return NULL;
}
