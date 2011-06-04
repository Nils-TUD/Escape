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

#ifndef VFSREQ_H_
#define VFSREQ_H_

#include <sys/common.h>

/* the states of a request */
#define REQ_STATE_WAITING		0
#define REQ_STATE_WAIT_DATA		1
#define REQ_STATE_FINISHED		2

/* an entry in the request-list */
typedef struct {
	tTid tid;
	sVFSNode *node;
	uint8_t state;
	uint val1;
	uint val2;
	size_t count;
	void *data;
	size_t dsize;
} sRequest;

/* a request-handler */
typedef void (*fReqHandler)(sVFSNode *node,const void *data,size_t size);

/**
 * Inits the vfs-requests
 */
void vfs_req_init(void);

/**
 * Sets the handler for the given message-id
 *
 * @param id the message-id
 * @param f the handler-function
 * @return true if successfull
 */
bool vfs_req_setHandler(tMsgId id,fReqHandler f);

/**
 * Sends the given message to the appropriate handler
 *
 * @param id the message-id
 * @param node the driver-node
 * @param data the message
 * @param size the size of the message
 */
void vfs_req_sendMsg(tMsgId id,sVFSNode *node,const void *data,size_t size);

/**
 * Allocates a new request-object with given properties
 *
 * @param node the vfs-node over which the request is handled
 * @param buffer optional, the buffer (stored in data)
 * @param size optional, the buffer-size (stored in dsize)
 * @return the request or NULL if not enough mem
 */
sRequest *vfs_req_get(sVFSNode *node,void *buffer,size_t size);

/**
 * Waits for the given request. You may get interrupted even if you don't allow signals since
 * a driver can be deregistered which leads to notifying all possibly affected threads. In this
 * case req->count will be ERR_DRIVER_DIED. Please check in this case if you are affected and
 * retry the request if not.
 *
 * @param req the request
 * @param allowSigs wether the thread should be interruptable by signals
 */
void vfs_req_waitForReply(sRequest *req,bool allowSigs);

/**
 * Searches for the request of the given node
 *
 * @param node the vfs-node over which the request is handled
 * @return the request or NULL
 */
sRequest *vfs_req_getByNode(const sVFSNode *node);

/**
 * Removes the request from the queue (so that it can't receive a msg anymore), but does not free it
 *
 * @param r the request
 */
void vfs_req_remove(sRequest *r);

/**
 * Removes the request from the queue and free's it
 *
 * @param r the request
 */
void vfs_req_free(sRequest *r);


#if DEBUGGING

/**
 * Prints all active requests
 */
void vfs_req_dbg_printAll(void);

/**
 * Prints the given request
 *
 * @param r the request
 */
void vfs_req_dbg_print(sRequest *r);

#endif

#endif /* VFSREQ_H_ */
