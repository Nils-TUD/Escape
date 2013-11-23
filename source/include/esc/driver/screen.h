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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

int screen_setCursor(int fd,gpos_t x,gpos_t y,int cursor);
int screen_joinShm(sScreenMode *mode,char **addr,const char *name,int type);
int screen_createShm(sScreenMode *mode,char **addr,const char *name,int type,uint perms);
void screen_destroyShm(sScreenMode *mode,char *addr,const char *name,int type);
ssize_t screen_collectModes(int fd,sScreenMode **modes);
int screen_findTextMode(int fd,uint cols,uint rows,sScreenMode *mode);
int screen_findGraphicsMode(int fd,gsize_t width,gsize_t height,gcoldepth_t bpp,sScreenMode *mode);
int screen_getMode(int fd,sScreenMode *mode);
int screen_setMode(int fd,int type,int mode,const char *shm,bool switchMode);
int screen_update(int fd,gpos_t x,gpos_t y,gsize_t width,gsize_t height);
ssize_t screen_getModeCount(int fd);
ssize_t screen_getModes(int fd,sScreenMode *modes,size_t count);

#ifdef __cplusplus
}
#endif
