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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/request.h>
#include <sys/vfs/server.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <esc/sllist.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>

static void vfs_drv_wait(sRequest *req);
static void vfs_drv_openReqHandler(sVFSNode *node,const void *data,u32 size);
static void vfs_drv_readReqHandler(sVFSNode *node,const void *data,u32 size);
static void vfs_drv_writeReqHandler(sVFSNode *node,const void *data,u32 size);

static sMsg msg;

void vfs_drv_init(void) {
	vfs_req_setHandler(MSG_DRV_OPEN_RESP,vfs_drv_openReqHandler);
	vfs_req_setHandler(MSG_DRV_READ_RESP,vfs_drv_readReqHandler);
	vfs_req_setHandler(MSG_DRV_WRITE_RESP,vfs_drv_writeReqHandler);
}

s32 vfs_drv_open(tPid pid,tFileNo file,sVFSNode *node,u32 flags) {
	s32 res;
	sRequest *req;

	/* if the driver doesn't implement open, its ok */
	if(!vfs_server_supports(node->parent,DRV_OPEN))
		return 0;

	/* get request; maybe we have to wait */
	req = vfs_req_getRequest(node,NULL,0);

	/* send msg to driver */
	msg.args.arg1 = flags;
	res = vfs_sendMsg(pid,file,MSG_DRV_OPEN,&msg,sizeof(msg.args));
	if(res < 0) {
		vfs_req_remRequest(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);
	res = (s32)req->count;
	vfs_req_remRequest(req);
	return res;
}

s32 vfs_drv_read(tPid pid,tFileNo file,sVFSNode *node,void *buffer,u32 offset,u32 count) {
	sRequest *req;
	sThread *t = thread_getRunning();
	volatile sVFSNode *n = node;
	void *data;
	s32 res;

	/* if the driver doesn't implement open, its an error */
	if(!vfs_server_supports(node->parent,DRV_READ))
		return ERR_UNSUPPORTED_OP;

	/* wait until data is readable */
	if(!vfs_server_isReadable(n->parent)) {
		if(!vfs_shouldBlock(file))
			return ERR_WOULD_BLOCK;
		ev_wait(t->tid,EVI_DATA_READABLE,(tEvObj)node);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			return ERR_INTERRUPTED;
		/* if we waked up and got no signal, the driver probably died */
		if(!vfs_server_isReadable(n->parent))
			return ERR_DRIVER_DIED;
	}

	/* get request; maybe we have to wait */
	req = vfs_req_getRequest(node,NULL,count);

	/* send msg to driver */
	msg.args.arg1 = offset;
	msg.args.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DRV_READ,&msg,sizeof(msg.args));
	if(res < 0) {
		vfs_req_remRequest(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);
	res = req->count;
	data = req->data;
	/* Better release the request before the memcpy so that it can be reused. Because memcpy might
	 * cause a page-fault which leads to swapping -> thread-switch. */
	vfs_req_remRequest(req);
	if(data) {
		memcpy(buffer,data,res);
		kheap_free(data);
	}
	return res;
}

s32 vfs_drv_write(tPid pid,tFileNo file,sVFSNode *node,const void *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	/* if the driver doesn't implement open, its an error */
	if(!vfs_server_supports(node->parent,DRV_WRITE))
		return ERR_UNSUPPORTED_OP;

	/* get request; maybe we have to wait */
	req = vfs_req_getRequest(node,NULL,0);

	/* send msg to driver */
	msg.args.arg1 = offset;
	msg.args.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DRV_WRITE,&msg,sizeof(msg.args));
	if(res < 0) {
		vfs_req_remRequest(req);
		return res;
	}
	/* now send data */
	res = vfs_sendMsg(pid,file,MSG_DRV_WRITE,buffer,count);
	if(res < 0) {
		vfs_req_remRequest(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);
	res = req->count;
	vfs_req_remRequest(req);
	return res;
}

void vfs_drv_close(tPid pid,tFileNo file,sVFSNode *node) {
	/* if the driver doesn't implement open, stop here */
	if(!vfs_server_supports(node->parent,DRV_CLOSE))
		return;

	vfs_sendMsg(pid,file,MSG_DRV_CLOSE,&msg,sizeof(msg.args));
}

static void vfs_drv_wait(sRequest *req) {
	/* repeat until it succeded or the driver died */
	volatile sRequest *r = req;
	do
		vfs_req_waitForReply(req,false);
	while((s32)r->count == ERR_INTERRUPTED);
}

static void vfs_drv_openReqHandler(sVFSNode *node,const void *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfs_req_getRequestByNode(node);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = (u32)rmsg->args.arg1;
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static void vfs_drv_readReqHandler(sVFSNode *node,const void *data,u32 size) {
	/* find the request for the node */
	sRequest *req = vfs_req_getRequestByNode(node);
	if(req != NULL) {
		sVFSNode *drv = node->parent;
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(!data || size < sizeof(rmsg->args) || (s32)rmsg->args.arg1 <= 0) {
				if(data && size >= sizeof(rmsg->args)) {
					vfs_server_setReadable(drv,rmsg->args.arg2);
					req->count = rmsg->args.arg1;
				}
				else {
					vfs_server_setReadable(drv,true);
					req->count = 0;
				}
				req->state = REQ_STATE_FINISHED;
				ev_wakeupThread(req->tid,EV_REQ_REPLY);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			/* set whether data is readable; do this here because a thread-switch may cause
			 * the driver to set that data is readable although arg2 was 0 here (= no data) */
			vfs_server_setReadable(drv,rmsg->args.arg2);
			req->count = MIN(req->dsize,rmsg->args.arg1);
			req->state = REQ_STATE_WAIT_DATA;
		}
		else if(req->state == REQ_STATE_WAIT_DATA) {
			/* ok, it's the data */
			if(data) {
				/* map the buffer we have to copy it to */
				req->data = (u32*)kheap_alloc(req->count);
				if(req->data)
					memcpy(req->data,data,req->count);
			}
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			ev_wakeupThread(req->tid,EV_REQ_REPLY);
		}
	}
}

static void vfs_drv_writeReqHandler(sVFSNode *node,const void *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfs_req_getRequestByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}
