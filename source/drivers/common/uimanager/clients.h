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
#include <esc/messages.h>

#include "keymap.h"

typedef struct {
	const char *name;
	sVTMode *modes;
	size_t modeCount;
} sBackend;

typedef struct {
	int id;
	int randid;
	sKeymap *map;
	sBackend *backend;
	sVTMode *backendMode;
	sVTPos cursor;
	char *backendShmName;
	int backendFd;
	enum {
		CONS_TYPE_NONE,
		CONS_TYPE_TEXT,
		CONS_TYPE_GRAPHICS,
	} type;
} sClient;

int cli_add(int id,sKeymap *map);
sClient *cli_getActive(void);
sClient *cli_get(int id);
void cli_next(void);
void cli_prev(void);
int cli_attach(int id,int randid);
void cli_detach(int id);
void cli_send(const void *msg,size_t size);
void cli_remove(int id);
