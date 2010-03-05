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
#include <vfs/vfs.h>
#include <vfs/driver.h>
#include <vfs/request.h>
#include <mem/kheap.h>
#include <mem/paging.h>
#include <task/thread.h>
#include <string.h>
#include <sllist.h>
#include <errors.h>

#include <messages.h>

static void vfsdrv_openReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static void vfsdrv_readReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static void vfsdrv_writeReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);

static sMsg msg;

void vfsdrv_init(void) {
	vfsreq_setHandler(MSG_DRV_OPEN_RESP,vfsdrv_openReqHandler);
	vfsreq_setHandler(MSG_DRV_READ_RESP,vfsdrv_readReqHandler);
	vfsreq_setHandler(MSG_DRV_WRITE_RESP,vfsdrv_writeReqHandler);
}

s32 vfsdrv_open(tTid tid,tFileNo file,sVFSNode *node,u32 flags) {
	s32 res;
	sRequest *req;

	/* if the driver doesn't implement open, its ok */
	if(!DRV_IMPL(node->parent->data.service.funcs,DRV_OPEN))
		return 0;

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
	u32 pcount,*frameNos;
	bool wasReadable;
	s32 res;

	/* if the driver doesn't implement open, its an error */
	if(!DRV_IMPL(node->parent->data.service.funcs,DRV_READ))
		return ERR_UNSUPPORTED_OP;

	/* wait until data is readable */
	while(n->parent->data.service.isEmpty) {
		thread_wait(tid,node->parent,EV_DATA_READABLE);
		thread_switchInKernel();
	}

	/* send msg to driver */
	msg.args.arg1 = offset;
	msg.args.arg2 = count;
	res = vfs_sendMsg(tid,file,MSG_DRV_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

#if 0
	/* get the frame-numbers which we'll map later to write the reply directly to the calling
	 * process */
	pcount = BYTES_2_PAGES(((u32)buffer & (PAGE_SIZE - 1)) + count);
	frameNos = kheap_alloc(pcount * sizeof(u32));
	if(frameNos == NULL)
		return ERR_NOT_ENOUGH_MEM;
	paging_getFrameNos(frameNos,(u32)buffer,count);

	/* wait for a reply */
	req = vfsreq_waitForReadReply(tid,count,frameNos,pcount,(u32)buffer % PAGE_SIZE);
#endif
	req = vfsreq_waitForReply(tid,buffer,count);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	if(req->readFrNos) {
		memcpy(buffer,req->readFrNos,req->count);
		kheap_free(req->readFrNos);
	}
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_write(tTid tid,tFileNo file,sVFSNode *node,const void *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	/* if the driver doesn't implement open, its an error */
	if(!DRV_IMPL(node->parent->data.service.funcs,DRV_WRITE))
		return ERR_UNSUPPORTED_OP;

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

void vfsdrv_close(tTid tid,tFileNo file,sVFSNode *node) {
	/* if the driver doesn't implement open, stop here */
	if(!DRV_IMPL(node->parent->data.service.funcs,DRV_CLOSE))
		return;

	vfs_sendMsg(tid,file,MSG_DRV_CLOSE,(u8*)&msg,sizeof(msg.args));
}

static void vfsdrv_openReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = (u32)rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_REQ_REPLY);
	}
}

static void vfsdrv_readReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			bool wasEmpty = node->data.service.isEmpty;
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(!data || size < sizeof(rmsg->args) || (s32)rmsg->args.arg1 <= 0) {
				if(data && size >= sizeof(rmsg->args)) {
					node->data.service.isEmpty = !rmsg->args.arg2;
					req->count = rmsg->args.arg1;
				}
				else {
					node->data.service.isEmpty = false;
					req->count = 0;
				}
				if(wasEmpty && !node->data.service.isEmpty)
					thread_wakeupAll(node,EV_DATA_READABLE | EV_RECEIVED_MSG);
				req->state = REQ_STATE_FINISHED;
				thread_wakeup(tid,EV_REQ_REPLY);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			/* set wether data is readable; do this here because a thread-switch may cause
			 * the service to set that data is readable although arg2 was 0 here (= no data) */
			node->data.service.isEmpty = !rmsg->args.arg2;
			req->count = MIN(req->dsize,rmsg->args.arg1);
			req->state = REQ_STATE_WAIT_DATA;
			if(wasEmpty && !node->data.service.isEmpty)
				thread_wakeupAll(node,EV_DATA_READABLE | EV_RECEIVED_MSG);
		}
		else {
			/* ok, it's the data */
			if(data) {
				/* map the buffer we have to copy it to */
				req->readFrNos = (u32*)kheap_alloc(req->count);
				if(req->readFrNos)
					memcpy(req->readFrNos,data,req->count);
			}
#if 0
			u8 *addr = (u8*)TEMP_MAP_AREA;
			paging_map(TEMP_MAP_AREA,req->readFrNos,req->readFrNoCount,PG_SUPERVISOR | PG_WRITABLE,true);
			memcpy(addr + req->readOffset,data,req->count);
			/* unmap it and free the frame-nos */
			paging_unmap(TEMP_MAP_AREA,req->readFrNoCount,false,false);
			kheap_free(req->readFrNos);
#endif
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			thread_wakeup(tid,EV_REQ_REPLY);
		}
	}
}

static void vfsdrv_writeReqHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_REQ_REPLY);
	}
}
