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

/* This is what the monitor reference in Object points to */
typedef struct Monitor {
	void* impl; /* for user-level monitors */
	Array devt; /* for internal monitors */
	tULock mon;
} Monitor;

#define MONPTR(h)       (&((Monitor *)(h)->monitor)->mon)

static tULock _monitor_critsec;

void _STI_monitor_staticctor() {
	/* nothing to do */
}

void _STD_monitor_staticdtor() {
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
	assert(h && h->monitor && !(((Monitor*)h->monitor)->impl));
	locku(MONPTR(h));
}

void _d_monitor_unlock(Object *h) {
	assert(h && h->monitor && !(((Monitor*)h->monitor)->impl));
	unlocku(MONPTR(h));
}
