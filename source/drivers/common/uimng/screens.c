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
#include <esc/mem.h>
#include <stdio.h>
#include <stdlib.h>

#include "screens.h"
#include "clients.h"
#include "header.h"

static sScreen *screens;
static size_t screenCount;

void screens_init(int cnt,char *names[]) {
	/* open screen-devices */
	screenCount = cnt;
	screens = (sScreen*)malloc(sizeof(sScreen) * screenCount);
	for(int i = 0; i < cnt; i++) {
		sScreen *scr = screens + i;
		scr->name = strdup(names[i]);
		if(scr->name == NULL)
			error("Unable to clone device name");
		int fd = open(scr->name,IO_READ | IO_WRITE | IO_MSGS);
		if(fd < 0)
			error("Unable to open '%s'",scr->name);

		/* get modes */
		scr->modeCount = screen_getModeCount(fd);
		scr->modes = (sScreenMode*)malloc(scr->modeCount * sizeof(sScreenMode));
		if(!scr->modes)
			error("Unable to allocate modes for '%s'",scr->name);
		if(screen_getModes(fd,scr->modes,scr->modeCount) < 0)
			error("Unable to get modes for '%s'",scr->name);
		close(fd);
	}
}

bool screens_find(int mid,sScreenMode **mode,sScreen **scr) {
	for(size_t i = 0; i < screenCount; ++i) {
		for(size_t j = 0; j < screens[i].modeCount; ++j) {
			if(screens[i].modes[j].id == mid) {
				*mode = screens[i].modes + j;
				*scr = screens + i;
				return true;
			}
		}
	}
	return false;
}

void screens_adjustMode(sScreenMode *mode) {
	if(mode->type & VID_MODE_TYPE_GUI) {
		mode->height -= header_getHeight(VID_MODE_TYPE_GUI);
		mode->guiHeaderSize += header_getHeight(VID_MODE_TYPE_GUI);
	}
	if(mode->type & VID_MODE_TYPE_TUI) {
		mode->rows -= header_getHeight(VID_MODE_TYPE_TUI);
		mode->tuiHeaderSize += header_getHeight(VID_MODE_TYPE_TUI);
	}
}

ssize_t screens_getModes(sScreenMode *modes,size_t n) {
	size_t count = 0;
	for(size_t i = 0; i < screenCount; ++i)
		count += screens[i].modeCount;

	if(n == 0)
		return count;
	if(n != count)
		return -EINVAL;
	if(!modes)
		return -ENOMEM;

	size_t pos = 0;
	for(size_t i = 0; i < screenCount; ++i) {
		memcpy(modes + pos,screens[i].modes,
			screens[i].modeCount * sizeof(sScreenMode));
		for(size_t j = 0; j < screens[i].modeCount; ++j)
			screens_adjustMode(modes + pos + j);
		pos += screens[i].modeCount;
	}
	return n;
}

int screens_setMode(sClient *cli,int type,int mid,const char *shm) {
	if(cli->screen) {
		close(cli->screenFd);
		munmap(cli->screenShm);
		free(cli->header);
		free(cli->screenShmName);
		cli->screen = NULL;
	}
	cli->screen = NULL;
	cli->screenMode = NULL;
	cli->screenShm = NULL;
	cli->screenShmName = NULL;
	cli->header = NULL;

	int res = -EINVAL;
	sScreenMode *mode;
	sScreen *scr;
	if(!screens_find(mid,&mode,&scr))
		goto error;

	/* open screen */
	cli->type = type;
	cli->screenFd = open(scr->name,IO_MSGS);
	if(cli->screenFd < 0) {
		res = cli->screenFd;
		goto error;
	}

	/* create header-line */
	cli->header = (char*)malloc(header_getSize(mode,type));
	if(cli->header == NULL) {
		res = -ENOMEM;
		goto errorClose;
	}

	/* open shm */
	res = screen_joinShm(mode,&cli->screenShm,shm,type);
	if(res < 0)
		goto errorHeader;

	/* set screen mode */
	if((res = screen_setMode(cli->screenFd,cli->type,mid,shm,true)) < 0)
		goto errorUnmap;
	cli->screen = scr;
	cli->screenMode = mode;
	cli->screenShmName = strdup(shm);
	return 0;

errorUnmap:
	munmap(cli->screenShm);
errorHeader:
	free(cli->header);
errorClose:
	close(cli->screenFd);
error:
	return res;
}
