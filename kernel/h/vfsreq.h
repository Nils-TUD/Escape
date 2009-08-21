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

#include "common.h"

/* an entry in the request-list */
typedef struct {
	tTid tid;
	bool finished;
	u32 val1;
	u32 val2;
	void *data;
	u32 count;
} sRequest;

/* a request-handler */
typedef void (*fReqHandler)(const u8 *data,u32 size);

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
 * @param data the message
 * @param size the size of the message
 */
void vfsreq_sendMsg(tMsgId id,const u8 *data,u32 size);

/**
 * Adds a request for the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL if not enough mem
 */
sRequest *vfsreq_addRequest(tTid tid);

/**
 * Waits for a reply
 *
 * @param tid the thread to block
 * @param req the request
 */
void vfsreq_waitForReply(tTid tid,sRequest *req);

/**
 * Searches for the request of the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL
 */
sRequest *vfsreq_getRequestByPid(tTid tid);

/**
 * Marks the given request as finished
 *
 * @param r the request
 */
void vfsreq_remRequest(sRequest *r);

#endif /* VFSREQ_H_ */
