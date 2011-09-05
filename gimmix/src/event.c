/**
 * $Id: event.c 217 2011-06-03 18:11:57Z nasmussen $
 */

#include <assert.h>

#include "common.h"
#include "mmix/mem.h"
#include "event.h"

typedef struct sEventHandler sEventHandler;
struct sEventHandler {
	sEventHandler *next;
	fEvent func;
};

static void ev_doFire(int event,const sEvArgs *args);

static sEventHandler *handler[EV_COUNT];

void ev_register(int event,fEvent func) {
	assert(event < (int)ARRAY_SIZE(handler));
	sEventHandler *p = handler[event];
	while(p && p->next)
		p = p->next;

	sEventHandler *h = (sEventHandler*)mem_alloc(sizeof(sEventHandler));
	h->func = func;
	h->next = NULL;
	if(p)
		p->next = h;
	else
		handler[event] = h;
}

void ev_unregister(int event,fEvent func) {
	assert(event < (int)ARRAY_SIZE(handler));
	sEventHandler *h = handler[event];
	sEventHandler *p = NULL;
	while(h != NULL) {
		if(h->func == func) {
			if(p)
				p->next = h->next;
			else
				handler[event] = h->next;
			mem_free(h);
			break;
		}
		p = h;
		h = h->next;
	}
}

void ev_fire(int event) {
	assert(event < (int)ARRAY_SIZE(handler));
	// no handler?
	if(handler[event] == NULL)
		return;

	sEvArgs args;
	args.event = event;
	ev_doFire(event,&args);
}

void ev_fire1(int event,octa arg1) {
	assert(event < (int)ARRAY_SIZE(handler));
	if(handler[event] == NULL)
		return;

	sEvArgs args;
	args.event = event;
	args.arg1 = arg1;
	ev_doFire(event,&args);
}

void ev_fire2(int event,octa arg1,octa arg2) {
	assert(event < (int)ARRAY_SIZE(handler));
	if(handler[event] == NULL)
		return;

	sEvArgs args;
	args.event = event;
	args.arg1 = arg1;
	args.arg2 = arg2;
	ev_doFire(event,&args);
}

static void ev_doFire(int event,const sEvArgs *args) {
	sEventHandler *h = handler[event];
	while(h != NULL) {
		h->func(args);
		h = h->next;
	}
}
