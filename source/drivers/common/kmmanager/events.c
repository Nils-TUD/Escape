/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <esc/esccodes.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "events.h"

/* a listener */
typedef struct {
	int fd;
	uchar flags;
	uchar key;
	uchar modifier;
} sEventListener;

/**
 * Searches for the given listener
 */
static sEventListener *events_find(int fd,uchar flags,uchar key,uchar modifier);

/* all announced listeners */
static sSLList *listener;

void events_init(void) {
	listener = sll_create();
	assert(listener);
}

bool events_send(sKmData *km) {
	sSLNode *n;
	sMsg msg;
	bool copied = false;
	for(n = sll_begin(listener); n != NULL; n = n->next) {
		sEventListener *l = (sEventListener*)n->data;
		/* modifiers equal and pressed/released? */
		if(l->modifier == km->modifier &&
			(((l->flags & KE_EV_RELEASED) && (km->modifier & STATE_BREAK)) ||
				((l->flags & KE_EV_PRESSED) && !(km->modifier & STATE_BREAK)))) {
			/* character/keycode equal? */
			if(((l->flags & KE_EV_KEYCODE) && l->key == km->keycode) ||
					((l->flags & KE_EV_CHARACTER) && l->key == km->character)) {
				if(!copied) {
					memcpy(&msg.data.d,km,sizeof(sKmData));
					copied = true;
				}
				send(l->fd,MSG_KM_EVENT,&msg,sizeof(msg.data));
			}
		}
	}
	return copied;
}

int events_add(int fd,uchar flags,uchar key,uchar modifier) {
	sEventListener *l;
	if(events_find(fd,flags,key,modifier) != NULL)
		return -EEXIST;

	l = (sEventListener*)malloc(sizeof(sEventListener));
	l->fd = fd;
	l->flags = flags;
	l->key = key;
	l->modifier = modifier;
	if(!sll_append(listener,l)) {
		free(l);
		return -ENOMEM;
	}
	return 0;
}

void events_remove(int fd,uchar flags,uchar key,uchar modifier) {
	sEventListener *l = events_find(fd,flags,key,modifier);
	if(l) {
		sll_removeFirstWith(listener,l);
		free(l);
	}
}

static sEventListener *events_find(int fd,uchar flags,uchar key,uchar modifier) {
	sSLNode *n;
	for(n = sll_begin(listener); n != NULL; n = n->next) {
		sEventListener *l = (sEventListener*)n->data;
		if(l->fd == fd && l->flags == flags && l->key == key && l->modifier == modifier)
			return l;
	}
	return NULL;
}
