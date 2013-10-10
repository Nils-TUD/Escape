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
#include <esc/driver/screen.h>
#include <esc/driver.h>
#include <esc/sllist.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "clients.h"

#define MAX_CLIENT_FDS					64

static sClient *clients[MAX_CLIENT_FDS];
static sSLList *clientList;
static int active = MAX_CLIENT_FDS;
static int activeIdx = MAX_CLIENT_FDS;

sClient *cli_getActive(void) {
	if(active != MAX_CLIENT_FDS)
		return clients[active];
	return NULL;
}

sClient *cli_get(int id) {
	assert(clients[id] != NULL);
	return clients[id];
}

static void cli_switch(int incr) {
	int oldActive = active;
	int oldMode = active == MAX_CLIENT_FDS ? -1 : clients[active]->screenMode->id;
	if(activeIdx == MAX_CLIENT_FDS)
		activeIdx = 0;
	else
		activeIdx = (activeIdx + incr) % sll_length(clientList);

	if(sll_length(clientList) > 0)
		active = ((sClient*)sll_get(clientList,activeIdx))->id;
	else
		activeIdx = active = MAX_CLIENT_FDS;

	if(active != MAX_CLIENT_FDS && active != oldActive) {
		sClient *cli = clients[active];
		assert(cli->screenMode);

		if(screen_setMode(cli->screenFd,cli->type,cli->screenMode->id,cli->screenShmName,
				oldMode != cli->screenMode->id) < 0)
			printe("[UIM] Unable to set mode");
		if(screen_setCursor(cli->screenFd,cli->cursor.x,cli->cursor.y,cli->cursor.cursor) < 0)
			printe("[UIM] Unable to set cursor");

		gsize_t w = cli->type == VID_MODE_TYPE_TUI ? cli->screenMode->cols : cli->screenMode->width;
		gsize_t h = cli->type == VID_MODE_TYPE_TUI ? cli->screenMode->rows : cli->screenMode->height;
		if(screen_update(cli->screenFd,0,0,w,h) < 0)
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
	if(active == MAX_CLIENT_FDS)
		return;
	send(clients[active]->id,MSG_UIM_EVENT,msg,size);
}

static int cli_getByRandId(int randid,sClient **cli) {
	*cli = NULL;
	for(int i = 0; i < MAX_CLIENT_FDS; ++i) {
		if(clients[i] && clients[i]->randid == randid) {
			if(*cli)
				return -EEXIST;
			*cli = clients[i];
		}
	}
	return *cli == NULL ? -EINVAL : 0;
}

int cli_add(int id,const char *keymap) {
	assert(clients[id] == NULL);
	clients[id] = (sClient*)malloc(sizeof(sClient));
	if(clients[id] == NULL)
		return -ENOMEM;

	/* generate a unique id */
	sClient *dummy;
	int randid;
	do {
		randid = rand();
	}
	while(cli_getByRandId(randid,&dummy) == 0);

	/* request keymap */
	clients[id]->randid = randid;
	clients[id]->map = km_request(keymap);
	if(clients[id]->map == NULL) {
		free(clients[id]);
		clients[id] = NULL;
		return -EINVAL;
	}

	clients[id]->id = id;
	clients[id]->screen = NULL;
	clients[id]->screenMode = NULL;
	clients[id]->screenShmName = NULL;
	clients[id]->screenFd = -1;
	clients[id]->cursor.x = 0;
	clients[id]->cursor.y = 0;
	clients[id]->cursor.cursor = CURSOR_DEFAULT;
	clients[id]->type = VID_MODE_TYPE_TUI;
	return 0;
}

int cli_attach(int id,int randid) {
	int res;
	sClient *cli;
	if((res = cli_getByRandId(randid,&cli)) != 0)
		return res;

	if(clientList == NULL && (clientList = sll_create()) == NULL)
		return -ENOMEM;

	clients[id] = cli;
	/* change id. we attached the event-channel and this one should receive them */
	clients[id]->id = id;
	active = id;
	activeIdx = sll_length(clientList);
	if(!sll_append(clientList,clients[id])) {
		clients[id] = NULL;
		return -ENOMEM;
	}
	return 0;
}

void cli_detach(int id) {
	if(clients[id]) {
		bool shouldSwitch = false;
		if(activeIdx != MAX_CLIENT_FDS && sll_get(clientList,activeIdx) == clients[id]) {
			active = MAX_CLIENT_FDS;
			activeIdx = MAX_CLIENT_FDS;
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
	for(int i = 0; i < MAX_CLIENT_FDS; ++i) {
		if(id != i && clients[i] == cli)
			clients[i] = NULL;
	}
	cli_detach(id);
	free(cli->screenShmName);
	close(cli->screenFd);
	free(cli);
}
