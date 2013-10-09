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
#include <esc/driver/video.h>
#include <esc/driver.h>
#include <esc/sllist.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "clients.h"

#define MAX_CLIENTS					64

static sClient *clients[MAX_CLIENTS];
static sSLList *clientList;
static int active = MAX_CLIENTS;
static int activeIdx = MAX_CLIENTS;

sClient *cli_getActive(void) {
	if(active != MAX_CLIENTS)
		return clients[active];
	return NULL;
}

sClient *cli_get(int id) {
	assert(clients[id] != NULL);
	return clients[id];
}

static void cli_switch(int incr) {
	int oldActive = active;
	int oldMode = active == MAX_CLIENTS ? -1 : clients[active]->backendMode->id;
	if(activeIdx == MAX_CLIENTS)
		activeIdx = 0;
	else
		activeIdx = (activeIdx + incr) % sll_length(clientList);

	if(sll_length(clientList) > 0)
		active = ((sClient*)sll_get(clientList,activeIdx))->id;
	else
		activeIdx = active = MAX_CLIENTS;

	if(active != MAX_CLIENTS && active != oldActive) {
		sClient *cli = clients[active];
		assert(cli->backendMode);

		if(video_setMode(cli->backendFd,cli->backendMode->id,cli->backendShmName,
				oldMode != cli->backendMode->id) < 0)
			printe("[UIM] Unable to set mode");
		if(video_setCursor(cli->backendFd,&cli->cursor) < 0)
			printe("[UIM] Unable to set cursor");
		if(video_update(cli->backendFd,0,0,cli->backendMode->width,cli->backendMode->height) < 0)
			printe("[UIM] Screen update failed");
	}
}

void cli_next(void) {
	cli_switch(+1);
}

void cli_prev(void) {
	cli_switch(-1);
}

void cli_send(const void *msg,size_t size) {
	if(active == MAX_CLIENTS)
		return;
	send(clients[active]->id,MSG_KM_EVENT,msg,size);
}

int cli_add(int id,sKeymap *map) {
	assert(clients[id] == NULL);
	clients[id] = (sClient*)malloc(sizeof(sClient));
	clients[id]->id = id;
	clients[id]->randid = rand();
	clients[id]->map = map;
	clients[id]->backend = NULL;
	clients[id]->backendMode = NULL;
	clients[id]->backendShmName = NULL;
	clients[id]->backendFd = -1;
	clients[id]->cursor.col = 0;
	clients[id]->cursor.row = 0;
	clients[id]->type = CONS_TYPE_NONE;
	return 0;
}

int cli_attach(int id,int randid) {
	sClient *cli = NULL;
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(clients[i] && clients[i]->randid == randid) {
			if(cli)
				return -EEXIST;
			cli = clients[i];
		}
	}

	if(clientList == NULL && (clientList = sll_create()) == NULL)
		return -ENOMEM;

	clients[id] = cli;
	clients[id]->id = id;
	active = id;
	activeIdx = sll_length(clientList);
	sll_append(clientList,clients[id]);
	return 0;
}

void cli_detach(int id) {
	if(clients[id]) {
		bool shouldSwitch = false;
		if(activeIdx != MAX_CLIENTS && sll_get(clientList,activeIdx) == clients[id]) {
			active = MAX_CLIENTS;
			activeIdx = MAX_CLIENTS;
			shouldSwitch = true;
		}
		sll_removeFirstWith(clientList,clients[id]);
		clients[id] = NULL;
		/* do that here because we need to remove us from the list first */
		if(shouldSwitch)
			cli_next();
	}
}

void cli_remove(int id) {
	sClient *cli = clients[id];
	assert(cli != NULL);
	for(int i = 0; i < MAX_CLIENTS; ++i) {
		if(id != i && clients[i] == cli)
			clients[i] = NULL;
	}
	cli_detach(id);
	free(cli->backendShmName);
	close(cli->backendFd);
	free(cli);
}
