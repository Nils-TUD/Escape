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
#include <proc.h>
#include <kheap.h>
#include <sched.h>
#include <kevent.h>
#include <vfs.h>
#include <vfsnode.h>
#include <vfsreal.h>
#include <vfsreq.h>
#include <util.h>
#include <string.h>
#include <errors.h>

#include <fsinterface.h>
#include <messages.h>

#define REQUEST_COUNT		1024
#define HANDLER_COUNT		32

/* the vfs-service-file */
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

void vfsreq_sendMsg(tMsgId id,tTid tid,const u8 *data,u32 size) {
	if(id < HANDLER_COUNT && handler[id])
		handler[id](tid,data,size);
}

sRequest *vfsreq_waitForReply(tTid tid) {
	u32 i;
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
	req->data = NULL;
	req->count = 0;

	/* wait */
	while(req->state != REQ_STATE_FINISHED) {
		thread_wait(tid,EV_RECEIVED_MSG);
		thread_switchInKernel();
	}
	return req;
}

sRequest *vfsreq_getRequestByPid(tTid tid) {
	u32 i;
	sRequest *req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(req->tid == tid)
			return req;
		req++;
	}
	return NULL;
}

void vfsreq_remRequest(sRequest *r) {
	r->tid = INVALID_TID;
}
