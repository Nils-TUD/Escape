/* D programming language runtime library
 * Public Domain
 * written by Walter Bright, Digital Mars
 * www.digitalmars.com
 *
 * This is written in C because nobody has written a pthreads interface
 * to D yet.
 */

#include <esc/common.h>
#include <esc/lock.h>
#include <esc/heap.h>
#include <assert.h>
#include "mars.h"

#define MONPTR(h)	(&((Monitor *)(h)->monitor)->mon)

/* This is what the monitor reference in Object points to */
typedef struct Monitor {
	Array delegates; /* for the notification system */
	u32 count; /* needed to allow multiple locks from the same thread */
	tULock mon;
} Monitor;

void _d_notify_release(Object *);
void _STI_monitor_staticctor(void);
void _STD_monitor_staticdtor(void);
void _d_monitorenter(Object *h);
void _d_monitorexit(Object *h);
void _d_monitorrelease(Object *h);

static tULock _monitor_critsec;

void _STI_monitor_staticctor(void) {
	/* nothing to do */
}

void _STD_monitor_staticdtor(void) {
	/* nothing to do */
}

void _d_monitorenter(Object *h) {
	if(!h->monitor) {
		Monitor *cs = (Monitor*)calloc(sizeof(Monitor),1);
		assert(cs);
		locku(&_monitor_critsec);
		/* if, in the meantime, another thread didn't set it */
		if(!h->monitor) {
			h->monitor = (void*)cs;
			cs->mon = 0;
			cs = NULL;
		}
		unlocku(&_monitor_critsec);
		/* if we didn't use it */
		if(cs)
			free(cs);
	}
	if(((Monitor*)h->monitor)->count == 0)
		locku(MONPTR(h));
	((Monitor*)h->monitor)->count++;
}

void _d_monitorexit(Object *h) {
	Monitor *m = (Monitor*)h->monitor;
	assert(h && m);
	if(--m->count == 0)
		unlocku(MONPTR(h));
}

/**
 * Called by garbage collector when Object is free'd.
 */
void _d_monitorrelease(Object *h) {
	if(h->monitor) {
		_d_notify_release(h);

		/* We can improve this by making a free list of monitors */
		free((void *)h->monitor);

		h->monitor = NULL;
	}
}
