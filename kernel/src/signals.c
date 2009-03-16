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
#include "../h/video.h"
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
static sHandler *sig_get(tPid pid,tSig signal);

/* to increase the speed of sig_hasSignal() store the total number of waiting signals */
u32 totalSigs = 0;
/* a linked list of handlers for each signal */
static sSLList *handler[SIG_COUNT - 1] = {NULL};

bool sig_canHandle(tSig signal) {
	/* we can't add a handler for SIG_KILL */
	return signal >= 1 && signal < SIG_COUNT;
}

s32 sig_setHandler(tPid pid,tSig signal,fSigHandler func) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h == NULL) {
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

void sig_unsetHandler(tPid pid,tSig signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		totalSigs -= h->sigCount;
		sll_removeFirst(handler[signal - 1],h);
		kheap_free(h);
	}
}

void sig_removeHandlerFor(tPid pid) {
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
				if(h->pid == pid) {
					totalSigs -= h->sigCount;
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

bool sig_hasSignal(tSig *sig,tPid *pid) {
	u32 i;
	sSLNode *n;
	sHandler *h;
	sSLList **list;

	/* no signals at all? */
	if(totalSigs == 0)
		return false;

	/* search through all signal-lists */
	list = handler;
	for(i = 0; i < SIG_COUNT - 1; i++) {
		if(*list != NULL) {
			for(n = sll_begin(*list); n != NULL; n = n->next) {
				h = (sHandler*)n->data;
				if(h->active == 0 && h->sigCount > 0) {
					*pid = h->pid;
					*sig = i + 1;
					return true;
				}
			}
		}
		list++;
	}
	return false;
}

bool sig_addSignalFor(tPid pid,tSig signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		totalSigs++;
		h->sigCount++;
		return !h->active;
	}
	return false;
}

tPid sig_addSignal(tSig signal) {
	sSLList *list;
	sHandler *h;
	sSLNode *n;
	tPid res = INVALID_PID;

	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	list = handler[signal - 1];
	if(list != NULL) {
		for(n = sll_begin(list); n != NULL; n = n->next) {
			h = (sHandler*)n->data;
			h->sigCount++;
			totalSigs++;
			/* remember first proc for direct notification */
			if(!h->active && res == INVALID_PID)
				res = h->pid;
		}
	}

	return res;
}

fSigHandler sig_startHandling(tPid pid,tSig signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL) {
		ASSERT(totalSigs > 0,"We don't have any signals");
		ASSERT(h->sigCount > 0,"Process %d hasn't got signal %d",pid,signal);
		h->sigCount--;
		h->active = 1;
		totalSigs--;
		return h->handler;
	}
	return NULL;
}

void sig_ackHandling(tPid pid,tSig signal) {
	sHandler *h;
	ASSERT(sig_canHandle(signal),"Unable to handle signal %d");

	h = sig_get(pid,signal);
	if(h != NULL)
		h->active = 0;
}

static sHandler *sig_get(tPid pid,tSig signal) {
	sSLList *list = handler[signal - 1];
	sHandler *h;
	sSLNode *n;
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		h = (sHandler*)n->data;
		if(h->pid == pid)
			return h;
	}
	return NULL;
}


#if DEBUGGING

u32 sig_dbg_getHandlerCount(void) {
	u32 i,c;
	sSLNode *n;
	sHandler *h;
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

cstring sig_dbg_getName(tSig signal) {
	static cstring names[] = {
		"SIG_KILL",
		"SIG_TERM",
		"SIG_ILL_INSTR",
		"SIG_SEGFAULT",
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
				vid_printf("\t\t(0x%08x): Pid=%d, handler=0x%x, active=%d, sigCount=%d\n",h,
						h->pid,h->handler,h->active,h->sigCount);
			}
		}
		list++;
	}
}

#endif
