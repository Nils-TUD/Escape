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
#include <vfsdrv.h>
#include <vfs.h>
#include <vfsreq.h>
#include <kheap.h>
#include <paging.h>
#include <thread.h>
#include <string.h>
#include <sllist.h>
#include <errors.h>

#include <messages.h>

static void vfsdrv_openReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsdrv_readReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsdrv_writeReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsdrv_ioctlReqHandler(tTid tid,const u8 *data,u32 size);

static sMsg msg;

void vfsdrv_init(void) {
	vfsreq_setHandler(MSG_DRV_OPEN_RESP,vfsdrv_openReqHandler);
	vfsreq_setHandler(MSG_DRV_READ_RESP,vfsdrv_readReqHandler);
	vfsreq_setHandler(MSG_DRV_WRITE_RESP,vfsdrv_writeReqHandler);
	vfsreq_setHandler(MSG_DRV_IOCTL_RESP,vfsdrv_ioctlReqHandler);
}

s32 vfsdrv_open(tTid tid,tFileNo file,sVFSNode *node,u32 flags) {
	s32 res;
	sRequest *req;

	UNUSED(node);

	/* send msg to driver */
	msg.args.arg1 = flags;
	res = vfs_sendMsg(tid,file,MSG_DRV_OPEN,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = (s32)req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_read(tTid tid,tFileNo file,sVFSNode *node,void *buffer,u32 offset,u32 count) {
	sRequest *req;
	volatile sVFSNode *n = node;
	s32 res;

	/* wait until data is readable */
	while(n->parent->data.service.isEmpty) {
		thread_wait(tid,EV_DATA_READABLE);
		thread_switchInKernel();
	}

	/* send msg to driver */
	msg.args.arg1 = offset;
	msg.args.arg2 = count;
	res = vfs_sendMsg(tid,file,MSG_DRV_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,buffer,count);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	/* store wether there is more data readable */
	node->parent->data.service.isEmpty = !req->val1;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_write(tTid tid,tFileNo file,sVFSNode *node,const void *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	UNUSED(node);

	/* send msg to driver */
	msg.args.arg1 = offset;
	msg.args.arg2 = count;
	res = vfs_sendMsg(tid,file,MSG_DRV_WRITE,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;
	/* now send data */
	res = vfs_sendMsg(tid,file,MSG_DRV_WRITE,(u8*)buffer,count);
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_ioctl(tTid tid,tFileNo file,sVFSNode *node,u32 cmd,void *data,u32 size) {
	sRequest *req;
	s32 res;

	UNUSED(node);

	if(data != NULL && size > sizeof(msg.data.d))
		return ERR_INVALID_ARGS;

	/* send msg to driver */
	msg.data.arg1 = cmd;
	msg.data.arg2 = data != NULL ? size : 0;
	if(data != NULL)
		memcpy(msg.data.d,data,size);
	res = vfs_sendMsg(tid,file,MSG_DRV_IOCTL,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = 0;
	if((s32)req->count < 0)
		res = req->count;
	if(res == 0 && req->data != NULL)
		memcpy(data,req->data,MIN(size,req->count));
	vfsreq_remRequest(req);
	return res;
}

void vfsdrv_close(tTid tid,tFileNo file,sVFSNode *node) {
	UNUSED(node);
	vfs_sendMsg(tid,file,MSG_DRV_CLOSE,(u8*)&msg,sizeof(msg.args));
}

static void vfsdrv_openReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = (u32)rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static void vfsdrv_readReqHandler(tTid tid,const u8 *data,u32 size) {
	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(size < sizeof(rmsg->args) || (s32)rmsg->args.arg1 <= 0) {
				req->val1 = 1;
				req->count = 0;
				req->state = REQ_STATE_FINISHED;
				thread_wakeup(tid,EV_RECEIVED_MSG);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			req->count = MIN(req->dsize,rmsg->args.arg1);
			req->val1 = rmsg->args.arg2;
			req->state = REQ_STATE_WAIT_DATA;
		}
		else {
			/* ok, it's the data */
			sThread *t = thread_getById(tid);
			/* map the buffer we have to copy it to */
			u8 *target = (u8*)paging_mapAreaOf(t->proc,(u32)req->data,req->count);
			memcpy(target,data,req->count);
			paging_unmapArea((u32)req->data,req->count);
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			thread_wakeup(tid,EV_RECEIVED_MSG);
		}
	}
}

static void vfsdrv_writeReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static void vfsdrv_ioctlReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->data.arg1;
		req->data = NULL;
		if(req->count > 0) {
			req->data = (void*)kheap_alloc(req->count);
			if(req->data != NULL)
				memcpy(req->data,rmsg->data.d,req->count);
		}
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}
