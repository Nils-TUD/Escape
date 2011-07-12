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
#include <esc/sllist.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <stdlib.h>
#include "listener.h"

typedef struct {
	inode_t client;
	msgid_t mid;
} sWinListener;

static int drvId;
static sSLList *list = NULL;

void listener_init(int id) {
	drvId = id;
	list = sll_create();
	if(!list)
		error("Unable to create window-listener-list");
}

bool listener_add(inode_t client,msgid_t mid) {
	sWinListener *l = (sWinListener*)malloc(sizeof(sWinListener));
	if(!l)
		return false;
	l->client = client;
	l->mid = mid;
	if(!sll_append(list,l)) {
		free(l);
		return false;
	}
	return true;
}

void listener_notify(msgid_t mid,const sMsg *msg,size_t size) {
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		sWinListener *l = (sWinListener*)n->data;
		if(l->mid == mid) {
			int fd = getClient(drvId,l->client);
			if(fd >= 0) {
				send(fd,mid,msg,size);
				close(fd);
			}
		}
	}
}

void listener_remove(inode_t client,msgid_t mid) {
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		sWinListener *l = (sWinListener*)n->data;
		if(l->client == client && l->mid == mid) {
			sll_removeFirstWith(list,l);
			return;
		}
	}
}
