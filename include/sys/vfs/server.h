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
 * @param name the node-name
 * @param flags the flags
 * @return the node
 */
sVFSNode *vfs_server_create(tPid pid,sVFSNode *parent,char *name,u32 flags);

/**
 * Tells the server that the given client has been removed. This way, it can reset the internal
 * state that stores which client will be served next.
 *
 * @param node the server-node
 * @param client the client-node
 */
void vfs_server_clientRemoved(sVFSNode *node,sVFSNode *client);

/**
 * @param node the server-node
 * @return true if its a terminal
 */
bool vfs_server_isterm(sVFSNode *node);

/**
 * @param node the server-node
 * @param funcs the functions to check
 * @return true if the server supports the given functions
 */
bool vfs_server_supports(sVFSNode *node,u32 funcs);

/**
 * @param node the server-node
 * @param id the msg-id to check
 * @return true if the server accepts the given message
 */
bool vfs_server_accepts(sVFSNode *node,u32 id);

/**
 * @param node the server-node
 * @return true if data can be read from the server (is available)
 */
bool vfs_server_isReadable(sVFSNode *node);

/**
 * Sets wether data is available
 *
 * @param node the server-node
 * @param readable the new value
 * @return 0 on success
 */
s32 vfs_server_setReadable(sVFSNode *node,bool readable);

/**
 * Searches for a client of the given server-node that should be served
 *
 * @param node the server-node
 * @param cont will be set to false (never to true!), if the caller should stop and use service
 * 	the returned client
 * @param retry will be set to true (never to false!), if the caller should check all driver-nodes
 * 	again, after the current loop is finished
 * @return the client to serve or NULL if there is none
 */
sVFSNode *vfs_server_getWork(sVFSNode *node,bool *cont,bool *retry);

#if DEBUGGING

/**
 * Prints the given server
 *
 * @param n the server-node
 */
void vfs_server_dbg_print(sVFSNode *n);

#endif

#endif /* SERVER_H_ */
