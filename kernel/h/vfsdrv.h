/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef VFSDRV_H_
#define VFSDRV_H_

#include "common.h"
#include "vfsnode.h"

void vfsdrv_init(void);

s32 vfsdrv_register(sVFSNode *node);

void vfsdrv_unregister(sVFSNode *node);

s32 vfsdrv_open(tTid tid,sVFSNode *node,u32 flags);

s32 vfsdrv_read(tTid tid,sVFSNode *node,void *buffer,u32 offset,u32 count);

s32 vfsdrv_write(tTid tid,sVFSNode *node,const void *buffer,u32 offset,u32 count);

s32 vfsdrv_ioctl(tTid tid,sVFSNode *node,u32 cmd,void *data,u32 size);

void vfsdrv_close(tTid tid,sVFSNode *node);

#endif /* VFSDRV_H_ */
