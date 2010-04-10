/**
 * Contains the implementation for object monitors.
 *
 * Copyright: Copyright Digital Mars 2000 - 2009.
 * License:   <a href="http://www.boost.org/LICENSE_1_0.txt">Boost License 1.0</a>.
 * Authors:   Walter Bright, Sean Kelly
 *
 *          Copyright Digital Mars 2000 - 2009.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
#include <esc/common.h>
#include <esc/lock.h>
#include <esc/heap.h>
#include <assert.h>
#include "mars.h"

#define MONPTR(h)       (&((Monitor *)(h)->monitor)->mon)

/* This is what the monitor reference in Object points to */
typedef struct Monitor {
	void* impl; /* for user-level monitors */
	Array devt; /* for internal monitors */
	u32 count;	/* needed to allow multiple locks from the same thread */
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
	if (!h->monitor) {
		cs = (Monitor *) calloc(sizeof(Monitor), 1);
		assert(cs);
		h->monitor = (void *) cs;
		cs = NULL;
	}
	unlocku(&_monitor_critsec);
	if (cs)
		free(cs);
}

void _d_monitor_destroy(Object *h) {
	assert(h && h->monitor && !(((Monitor*)h->monitor)->impl));
	free((void *) h->monitor);
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
