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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/listeners.h>
#include <sys/mem/kheap.h>
#include <esc/sllist.h>
#include <errors.h>

#define LISTEN_MAP_SIZE		64

/* a listener */
typedef struct {
	sVFSNode *node;
	u32 events;
	fVFSListener listener;
} sListener;

/* a hashmap with all listeners; key is (nodeNo % LISTEN_MAP_SIZE) */
static sSLList *listeners[LISTEN_MAP_SIZE];

s32 vfsl_add(sVFSNode *node,u32 events,fVFSListener listener) {
	tInodeNo nodeNo = vfsn_getNodeNo(node);
	sSLList *list;
	sListener *l;
	/* just directories are supported atm */
	if(!MODE_IS_DIR(node->mode))
		return ERR_NO_DIRECTORY;

	/* get corresponding list */
	list = listeners[nodeNo % LISTEN_MAP_SIZE];
	if(list == NULL) {
		list = listeners[nodeNo % LISTEN_MAP_SIZE] = sll_create();
		if(list == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* alloc mem and append */
	l = (sListener*)kheap_alloc(sizeof(sListener));
	if(l == NULL)
		return ERR_NOT_ENOUGH_MEM;
	l->events = events;
	l->node = node;
	l->listener = listener;
	if(!sll_append(list,l)) {
		kheap_free(l);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

void vfsl_remove(sVFSNode *node,u32 events,fVFSListener listener) {
	tInodeNo nodeNo = vfsn_getNodeNo(node);
	sListener *l;
	sSLNode *n,*t,*p;
	sSLList *list = listeners[nodeNo % LISTEN_MAP_SIZE];
	if(list == NULL)
		return;

	/* remove all matching listeners */
	p = NULL;
	for(n = sll_begin(list); n != NULL; ) {
		l = (sListener*)n->data;
		/* remove complete listener? (when it is announced for no other event anymore) */
		if(l->node == node && l->listener == listener && (l->events & ~events) == 0) {
			t = n->next;
			kheap_free(l);
			sll_removeNode(list,n,p);
			n = t;
		}
		else {
			/* just remove the events, but leave the listener */
			if(l->node == node && l->listener == listener)
				l->events &= ~events;
			p = n;
			n = n->next;
		}
	}
}

void vfsl_notify(sVFSNode *node,u32 event) {
	tInodeNo nodeNo = vfsn_getNodeNo(node->parent);
	sListener *l;
	sSLNode *n;
	sSLList *list = listeners[nodeNo % LISTEN_MAP_SIZE];
	if(list == NULL)
		return;

	/* invoke all listeners for that event */
	for(n = sll_begin(list); n != NULL; n = n->next) {
		l = (sListener*)n->data;
		if(l->node == node->parent && (l->events & event))
			l->listener(node,event);
	}
}
