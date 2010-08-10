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

#define REQ_STATE_WAITING		0
#define REQ_STATE_WAIT_DATA		1
#define REQ_STATE_FINISHED		2

/* an entry in the request-list */
typedef struct {
	tTid tid;
	u8 state;
	u32 val1;
	u32 val2;
	u32 count;
	void *data;
	u32 dsize;
	u32 *readFrNos;
	u32 readFrNoCount;
	u32 readOffset;
} sRequest;

/* a request-handler */
typedef void (*fReqHandler)(tTid tid,sVFSNode *node,const u8 *data,u32 size);

/**
 * Inits the vfs-requests
 */
void vfsreq_init(void);

/**
 * Sets the handler for the given message-id
 *
 * @param id the message-id
 * @param f the handler-function
 * @return true if successfull
 */
bool vfsreq_setHandler(tMsgId id,fReqHandler f);

/**
 * Sends the given message to the appropriate handler
 *
 * @param id the message-id
 * @param node the driver-node
 * @param tid the receiver-tid
 * @param data the message
 * @param size the size of the message
 */
void vfsreq_sendMsg(tMsgId id,sVFSNode *node,tTid tid,const u8 *data,u32 size);

/**
 * Allocates a new request-object with given properties
 *
 * @param tid the thread to block
 * @param buffer optional, the buffer (stored in data)
 * @param size optional, the buffer-size (stored in dsize)
 * @return the request or NULL if not enough mem
 */
sRequest *vfsreq_getRequest(tTid tid,void *buffer,u32 size);

/**
 * Like vfsreq_getRequest(), but intended for the driver-function read()
 *
 * @param tid the thread to block
 * @param bufSize the buffer-size (stored in dsize)
 * @param frameNos the array of frame-numbers which have been saved for later mapping
 * @param frameNoCount the number of frame-numbers
 * @param offset the offset in the first page where to copy the data to
 * @return the request or NULL if not enough mem
 */
sRequest *vfsreq_getReadRequest(tTid tid,u32 bufSize,u32 *frameNos,u32 frameNoCount,u32 offset);

/**
 * Waits for the given request. You may get interrupted even if you don't allow signals since
 * a driver can be deregistered which leads to notifying all possibly affected threads. In this
 * case req->count will be ERR_DRIVER_DIED. Please check in this case if you are affected and
 * retry the request if not.
 *
 * @param req the request
 * @param allowSigs wether the thread should be interruptable by signals
 */
void vfsreq_waitForReply(sRequest *req,bool allowSigs);

/**
 * Searches for the request of the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL
 */
sRequest *vfsreq_getRequestByTid(tTid tid);

/**
 * Marks the given request as finished
 *
 * @param r the request
 */
void vfsreq_remRequest(sRequest *r);

#if DEBUGGING

/**
 * Prints the given request
 *
 * @param r the request
 */
void vfsreq_dbg_print(sRequest *r);

#endif

#endif /* VFSREQ_H_ */
