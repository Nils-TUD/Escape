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

#ifndef VFSINFO_H_
#define VFSINFO_H_

#include <sys/common.h>

/**
 * Creates the vfs-info nodes
 */
void vfs_info_init(void);

/**
 * The trace-read-handler
 */
s32 vfs_info_traceReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The proc-read-handler
 */
s32 vfs_info_procReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The thread-read-handler
 */
s32 vfs_info_threadReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The regions-read-handler
 */
s32 vfs_info_regionsReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

/**
 * The virtual-memory-read-handler
 */
s32 vfs_info_virtMemReadHandler(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);

#endif /* VFSINFO_H_ */
