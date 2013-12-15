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
#include <esc/mem.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "clients.h"
#include "header.h"

#define MAX_CLIENT_FDS					64

static sClient *clients[MAX_CLIENT_FDS];
static sClient *idx2cli[MAX_CLIENTS];
static size_t cliCount = 0;
static size_t active = MAX_CLIENT_FDS;
static size_t activeIdx = MAX_CLIENT_FDS;

bool cli_isActive(size_t idx) {
	return activeIdx == idx;
}

bool cli_exists(size_t idx) {
	return idx2cli[idx] != NULL;
}

sClient *cli_getActive(void) {
	if(active != MAX_CLIENT_FDS)
		return clients[active];
	return NULL;
}

sClient *cli_get(int id) {
	assert(clients[id] != NULL);
	return clients[id];
}

size_t cli_getCount(void) {
	return cliCount;
}

static int cli_getOldMode(void) {
	if(active != MAX_CLIENT_FDS && clients[active]->screenMode)
		return clients[active]->screenMode->id;
	return -1;
}

void cli_reactivate(int oldMode) {
	sClient *cli = clients[active];
	assert(cli->screenMode);

	if(screen_setMode(cli->screenFd,cli->type,cli->screenMode->id,cli->screenShmName,
			oldMode != cli->screenMode->id) < 0)
		printe("Unable to set mode");
	if(screen_setCursor(cli->screenFd,cli->cursor.x,cli->cursor.y,cli->cursor.cursor) < 0)
		printe("Unable to set cursor");

	gsize_t w = cli->type == VID_MODE_TYPE_TUI ? cli->screenMode->cols : cli->screenMode->width;
	gsize_t h = cli->type == VID_MODE_TYPE_TUI ? cli->screenMode->rows : cli->screenMode->height;
	gsize_t dw,dh;
	header_update(cli,&dw,&dh);
	if(screen_update(cli->screenFd,0,0,w,h) < 0)
		printe("Screen update failed");
}

static void cli_switch(int incr) {
	size_t oldActive = active;
	int oldMode = cli_getOldMode();
	if(activeIdx == MAX_CLIENT_FDS)
		activeIdx = 0;
	if(cliCount > 0) {
		do {
			activeIdx = (activeIdx + incr) % MAX_CLIENTS;
		}
		while(idx2cli[activeIdx] == NULL || idx2cli[activeIdx]->screenMode == NULL);
		active = idx2cli[activeIdx]->id;
	}
	else
		activeIdx = active = MAX_CLIENT_FDS;

	if(active != MAX_CLIENT_FDS && active != oldActive)
		cli_reactivate(oldMode);
}

void cli_next(void) {
	cli_switch(+1);
}

void cli_prev(void) {
	cli_switch(-1);
}

void cli_switchTo(size_t idx) {
	assert(idx < MAX_CLIENTS);
	if(idx != activeIdx && idx2cli[idx] != NULL) {
		int oldMode = cli_getOldMode();
		activeIdx = idx;
		active = idx2cli[activeIdx]->id;
		cli_reactivate(oldMode);
	}
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
	clients[id]->screenShm = NULL;
	clients[id]->screenShmName = NULL;
	clients[id]->screenFd = -1;
	clients[id]->header = NULL;
	clients[id]->cursor.x = 0;
	clients[id]->cursor.y = 0;
	clients[id]->cursor.cursor = CURSOR_DEFAULT;
	clients[id]->type = VID_MODE_TYPE_TUI;
	clients[id]->idx = -1;
	return 0;
}

int cli_attach(int id,int randid) {
	int res;
	sClient *cli;
	if((res = cli_getByRandId(randid,&cli)) != 0)
		return res;
	if(cliCount >= MAX_CLIENTS)
		return -EBUSY;

	clients[id] = cli;
	/* change id. we attached the event-channel and this one should receive them */
	clients[id]->id = id;
	active = id;

	for(size_t i = 0; i < MAX_CLIENTS; ++i) {
		if(idx2cli[i] == NULL) {
			idx2cli[i] = cli;
			cli->idx = i;
			activeIdx = i;
			cliCount++;
			break;
		}
	}
	return 0;
}

void cli_detach(int id) {
	if(clients[id] && clients[id]->idx != -1) {
		assert(idx2cli[clients[id]->idx] == clients[id]);
		bool shouldSwitch = false;
		if(activeIdx != MAX_CLIENT_FDS && idx2cli[activeIdx] == clients[id]) {
			active = MAX_CLIENT_FDS;
			activeIdx = MAX_CLIENT_FDS;
			shouldSwitch = true;
		}
		idx2cli[clients[id]->idx] = NULL;
		clients[id] = NULL;
		cliCount--;
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
	if(cli->screenShm)
		munmap(cli->screenShm);
	free(cli->header);
	free(cli->screenShmName);
	close(cli->screenFd);
	free(cli);
}
