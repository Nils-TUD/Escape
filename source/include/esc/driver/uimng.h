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

int uimng_getId(int fd);
int uimng_attach(int fd,int id);
int uimng_getMode(int fd);
int uimng_setMode(int fd,int mode,const char *shm);
int uimng_setCursor(int fd,const sVTPos *pos);
int uimng_update(int fd,uint start,uint count);
ssize_t uimng_getModeCount(int fd);
int uimng_getModes(int fd,sVTMode *modes,size_t count);

#ifdef __cplusplus
}
#endif
