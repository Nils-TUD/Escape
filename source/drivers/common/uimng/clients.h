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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>

#include "keymap.h"

#define MAX_CLIENTS						8	/* we have 8 ui groups atm */

typedef struct {
	const char *name;
	sScreenMode *modes;
	size_t modeCount;
} sScreen;

typedef struct {
	int id;
	int idx;
	int randid;
	sKeymap *map;
	sScreen *screen;
	sScreenMode *screenMode;
	char *screenShm;
	char *screenShmName;
	char *header;
	int screenFd;
	struct {
		gpos_t x;
		gpos_t y;
		int cursor;
	} cursor;
	int type;
} sClient;

int cli_add(int id,const char *keymap);
bool cli_isActive(size_t idx);
bool cli_exists(size_t idx);
sClient *cli_getActive(void);
sClient *cli_get(int id);
size_t cli_getCount(void);
void cli_reactivate(sClient *cli,sClient *old,int oldMode);
void cli_next(void);
void cli_prev(void);
void cli_switchTo(size_t idx);
int cli_attach(int id,int randid);
void cli_detach(int id);
void cli_send(const void *msg,size_t size);
void cli_remove(int id);
