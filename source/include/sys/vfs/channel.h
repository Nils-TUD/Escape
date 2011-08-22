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

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <sys/common.h>
#include <sys/vfs/vfs.h>

/**
 * Creates a new channel for given process
 *
 * @param pid the process-id
 * @param parent the parent-node
 * @return the created node or NULL
 */
sVFSNode *vfs_chan_create(pid_t pid,sVFSNode *parent);

/**
 * Locks the given channel. This is only intended for sending messages to this channel.
 *
 * @param node the channel-node
 */
void vfs_chan_lock(sVFSNode *node);

/**
 * Unlocks the given channel
 *
 * @param node the channel-node
 */
void vfs_chan_unlock(sVFSNode *node);

/**
 * Checks whether the given channel has a reply for the client
 *
 * @param node the channel-node
 * @return true if so
 */
bool vfs_chan_hasReply(const sVFSNode *node);

/**
 * Checks whether the given channel has work to do for the server
 *
 * @param node the channel-node
 * @return true if so
 */
bool vfs_chan_hasWork(const sVFSNode *node);

/**
 * Sends the given message to the channel (UNLOCKED!)
 *
 * @param pid the process-id
 * @param file the file
 * @param n the channel-node
 * @param id the message-id
 * @param data the message-data
 * @param size the data-size
 * @return 0 on success
 */
ssize_t vfs_chan_send(pid_t pid,file_t file,sVFSNode *n,msgid_t id,const void *data,size_t size);

/**
 * Receives a message from the channel (LOCKED!)
 *
 * @param pid the process-id
 * @param file the file
 * @param n the channel-node
 * @param id will be set to the message-id (if not NULL)
 * @param data the buffer to write the message to
 * @param size the size of the buffer
 * @return the number of written bytes on success
 */
ssize_t vfs_chan_receive(pid_t pid,file_t file,sVFSNode *node,msgid_t *id,void *data,size_t size);

/**
 * Prints the given channel
 *
 * @param n the channel-node
 */
void vfs_chan_print(const sVFSNode *n);

#endif /* CHANNEL_H_ */
