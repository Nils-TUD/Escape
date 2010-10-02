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

#ifndef SERVER_H_
#define SERVER_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>

/**
 * Creates a server-node
 *
 * @param pid the process-id to use
 * @param parent the parent-node
 * @param prev the previous node
 * @param name the node-name
 * @param flags the flags
 * @return the node
 */
sVFSNode *vfs_server_create(tPid pid,sVFSNode *parent,char *name,u32 flags);

void vfs_server_clientRemoved(sVFSNode *node,sVFSNode *client);
bool vfs_server_isterm(sVFSNode *node);
bool vfs_server_supports(sVFSNode *node,u32 funcs);
bool vfs_server_accepts(sVFSNode *node,u32 id);
bool vfs_server_isReadable(sVFSNode *node);
void vfs_server_setReadable(sVFSNode *node,bool readable);
sVFSNode *vfs_server_getWork(sVFSNode *node,bool *cont,bool *retry);

#if DEBUGGING

void vfs_server_dbg_print(sVFSNode *n);

#endif

#endif /* SERVER_H_ */
