/* D programming language runtime library
 * Public Domain
 * written by Walter Bright, Digital Mars
 * www.digitalmars.com
 *
 * This is written in C because nobody has written a pthreads interface
 * to D yet.
 */

#include <esc/common.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <assert.h>
#include "mars.h"

#define MONPTR(h)       (&((Monitor *)(h)->monitor)->mon)

/* This is what the monitor reference in Object points to */
typedef struct Monitor {
	void* impl; /* for user-level monitors */
	Array devt; /* for internal monitors */
	size_t count; /* needed to allow multiple locks from the same thread */
	tULock mon;
} Monitor;

void _STI_monitor_staticctor(void);
void _STD_monitor_staticdtor(void);
void _d_monitor_create(Object *h);
void _d_monitor_destroy(Object *h);
int _d_monitor_lock(Object *h);
void _d_monitor_unlock(Object *h);

static tULock _monitor_critsec;

void _STI_monitor_staticctor(void) {
	/* nothing to do */
}

void _STD_monitor_staticdtor(void) {
	/* nothing to do */
}

void _d_monitor_create(Object *h) {
	/*
	 * NOTE: Assume this is only called when h->monitor is null prior to the
	 * call.  However, please note that another thread may call this function
	 * at the same time, so we can not assert this here.  Instead, try and
	 * create a lock, and if one already exists then forget about it.
	 */

	assert(h);
	Monitor *cs = NULL;
	locku(&_monitor_critsec);
	if(!h->monitor) {
		cs = (Monitor *)calloc(sizeof(Monitor),1);
		assert(cs);
		cs->mon = 0;
		h->monitor = (void *)cs;
		cs = NULL;
	}
	unlocku(&_monitor_critsec);
	if(cs)
		free(cs);
}

void _d_monitor_destroy(Object *h) {
	assert(h && h->monitor && !(((Monitor*)h->monitor)->impl));
	free((void *)h->monitor);
	h->monitor = NULL;
}

int _d_monitor_lock(Object *h) {
	Monitor *m = (Monitor*)h->monitor;
	assert(h && m && !m->impl);
	if(m->count == 0)
		locku(MONPTR(h));
	m->count++;
	return 0;
}

void _d_monitor_unlock(Object *h) {
	Monitor *m = (Monitor*)h->monitor;
	assert(h && m && !m->impl);
	if(--m->count == 0)
		unlocku(MONPTR(h));
}
