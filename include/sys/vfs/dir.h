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

#ifndef DIR_H_
#define DIR_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>

sVFSNode *vfs_dir_create(tPid pid,sVFSNode *parent,char *name);
s32 vfs_dir_read(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

#endif /* DIR_H_ */
