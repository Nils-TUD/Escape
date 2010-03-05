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

#include <common.h>
#include <task/proc.h>
#include <task/sched.h>
#include <mem/kheap.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/real.h>
#include <vfs/request.h>
#include <kevent.h>
#include <util.h>
#include <string.h>
#include <errors.h>

#include <fsinterface.h>
#include <messages.h>

#define REQUEST_COUNT		1024
#define HANDLER_COUNT		32

/**
 * The internal function to add a request and wait for the reply
 */
static sRequest *vfsreq_waitForReplyIntern(tTid tid,void *buffer,u32 size,u32 *frameNos,
		u32 frameNoCount,u32 offset);

/* the vfs-driver-file */
static sRequest requests[REQUEST_COUNT];
static fReqHandler handler[HANDLER_COUNT] = {NULL};

void vfsreq_init(void) {
	u32 i;
	sRequest *req;

	req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		req->tid = INVALID_TID;
		req++;
	}
}

bool vfsreq_setHandler(tMsgId id,fReqHandler f) {
	if(id >= HANDLER_COUNT || handler[id] != NULL)
		return false;
	handler[id] = f;
	return true;
}

void vfsreq_sendMsg(tMsgId id,sVFSNode *node,tTid tid,const u8 *data,u32 size) {
	if(id < HANDLER_COUNT && handler[id])
		handler[id](tid,node,data,size);
}

sRequest *vfsreq_waitForReply(tTid tid,void *buffer,u32 size) {
	return vfsreq_waitForReplyIntern(tid,buffer,size,NULL,0,0);
}

sRequest *vfsreq_waitForReadReply(tTid tid,u32 bufSize,u32 *frameNos,u32 frameNoCount,u32 offset) {
	return vfsreq_waitForReplyIntern(tid,NULL,bufSize,frameNos,frameNoCount,offset);
}

static sRequest *vfsreq_waitForReplyIntern(tTid tid,void *buffer,u32 size,u32 *frameNos,
		u32 frameNoCount,u32 offset) {
	u32 i;
	volatile sRequest *vreq;
	sRequest *req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(req->tid == INVALID_TID)
			break;
		req++;
	}
	if(i == REQUEST_COUNT)
		return NULL;

	req->tid = tid;
	req->state = REQ_STATE_WAITING;
	req->val1 = 0;
	req->val2 = 0;
	req->data = buffer;
	req->dsize = size;
	req->readFrNos = frameNos;
	req->readFrNoCount = frameNoCount;
	req->readOffset = offset;
	req->count = 0;

	/* wait */
	vreq = req;
	while(vreq->state != REQ_STATE_FINISHED) {
		thread_wait(tid,0,EV_REQ_REPLY);
		thread_switchInKernel();
	}
	return req;
}

sRequest *vfsreq_getRequestByPid(tTid tid) {
	u32 i;
	sRequest *req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(req->tid == tid) {
			/* the thread may have been terminated... */
			if(thread_getById(tid) == NULL) {
				vfsreq_remRequest(req);
				return NULL;
			}
			return req;
		}
		req++;
	}
	return NULL;
}

void vfsreq_remRequest(sRequest *r) {
	r->tid = INVALID_TID;
}
