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
ssize_t vfs_info_traceReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The proc-read-handler
 */
ssize_t vfs_info_procReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The thread-read-handler
 */
ssize_t vfs_info_threadReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The regions-read-handler
 */
ssize_t vfs_info_regionsReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

/**
 * The maps-read-handler
 */
ssize_t vfs_info_mapsReadHandler(pid_t pid,sFile *file,sVFSNode *node,USER void *buffer,
		off_t offset,size_t count);

/**
 * The virtual-memory-read-handler
 */
ssize_t vfs_info_virtMemReadHandler(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
		off_t offset,size_t count);

#endif /* VFSINFO_H_ */
