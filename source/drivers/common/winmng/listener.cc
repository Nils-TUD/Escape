/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/sllist.h>
#include <sys/driver.h>
#include <sys/thread.h>
#include <sys/sync.h>
#include <sys/io.h>
#include <stdlib.h>
#include "listener.h"

typedef struct {
	int client;
	ev_type type;
} sWinListener;

static tUserSem usem;
static int drvId;
static sSLList list;

void listener_init(int id) {
	drvId = id;
	sll_init(&list,malloc,free);
	if(usemcrt(&usem,1) < 0)
		error("Unable to create listener-lock");
}

bool listener_add(int client,ev_type type) {
	if(type != ev_type::TYPE_CREATED && type != ev_type::TYPE_DESTROYED &&
		type != ev_type::TYPE_ACTIVE)
		return false;

	sWinListener *l = (sWinListener*)malloc(sizeof(sWinListener));
	if(!l)
		return false;
	l->client = client;
	l->type = type;
	usemdown(&usem);
	bool res = sll_append(&list,l);
	usemup(&usem);
	if(!res) {
		free(l);
		return false;
	}
	return true;
}

void listener_notify(const ipc::WinMngEvents::Event *ev) {
	usemdown(&usem);
	for(sSLNode *n = sll_begin(&list); n != NULL; n = n->next) {
		sWinListener *l = (sWinListener*)n->data;
		if(l->type == ev->type)
			send(l->client,MSG_WIN_EVENT,ev,sizeof(*ev));
	}
	usemup(&usem);
}

void listener_remove(int client,ev_type type) {
	usemdown(&usem);
	for(sSLNode *n = sll_begin(&list); n != NULL; n = n->next) {
		sWinListener *l = (sWinListener*)n->data;
		if(l->client == client && l->type == type) {
			sll_removeFirstWith(&list,l);
			free(l);
			break;
		}
	}
	usemup(&usem);
}

void listener_removeAll(int client) {
	usemdown(&usem);
	sSLNode *p = NULL;
	for(sSLNode *n = sll_begin(&list); n != NULL; ) {
		sWinListener *l = (sWinListener*)n->data;
		if(l->client == client) {
			sSLNode *next = n->next;
			sll_removeNode(&list,n,p);
			free(l);
			n = next;
		}
		else {
			p = n;
			n = n->next;
		}
	}
	usemup(&usem);
}
