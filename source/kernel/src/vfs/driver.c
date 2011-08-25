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

#include <sys/common.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/request.h>
#include <sys/vfs/server.h>
#include <sys/vfs/channel.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <esc/sllist.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>

static void vfs_drv_wait(sRequest *req);
static void vfs_drv_openReqHandler(sVFSNode *node,const void *data,size_t size);
static void vfs_drv_readReqHandler(sVFSNode *node,const void *data,size_t size);
static void vfs_drv_writeReqHandler(sVFSNode *node,const void *data,size_t size);

void vfs_drv_init(void) {
	vfs_req_setHandler(MSG_DRV_OPEN_RESP,vfs_drv_openReqHandler);
	vfs_req_setHandler(MSG_DRV_READ_RESP,vfs_drv_readReqHandler);
	vfs_req_setHandler(MSG_DRV_WRITE_RESP,vfs_drv_writeReqHandler);
}

ssize_t vfs_drv_open(pid_t pid,file_t file,sVFSNode *node,uint flags) {
	ssize_t res;
	sRequest *req;
	sArgsMsg msg;

	/* if the driver doesn't implement open, its ok */
	if(!vfs_server_supports(node->parent,DRV_OPEN))
		return 0;

	/* we have to ensure that the position of our request and the position of the message we're
	 * going to send is the same. i.e. we have to prevent that somebody else sends a message
	 * to this channel in the meanwhile */
	vfs_chan_lock(node);
	/* get request */
	req = vfs_req_get(node,NULL,0);
	if(!req) {
		vfs_chan_unlock(node);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* send msg to driver */
	msg.arg1 = flags;
	res = vfs_sendMsg(pid,file,MSG_DRV_OPEN,&msg,sizeof(msg));
	vfs_chan_unlock(node);
	if(res < 0) {
		vfs_req_free(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);
	res = (ssize_t)req->count;
	vfs_req_free(req);
	return res;
}

ssize_t vfs_drv_read(pid_t pid,file_t file,sVFSNode *node,USER void *buffer,off_t offset,
		size_t count) {
	sRequest *req;
	void *data;
	ssize_t res;
	sArgsMsg msg;
	sWaitObject obj;

	/* if the driver doesn't implement open, its an error */
	if(!vfs_server_supports(node->parent,DRV_READ))
		return ERR_UNSUPPORTED_OP;

	/* wait until there is data available, if necessary */
	obj.events = EV_DATA_READABLE;
	obj.object = file;
	res = vfs_waitFor(&obj,1,vfs_shouldBlock(file),KERNEL_PID,0);
	if(res < 0)
		return res;

	/* get request */
	vfs_chan_lock(node);
	req = vfs_req_get(node,NULL,count);
	if(!req) {
		vfs_chan_unlock(node);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* send msg to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DRV_READ,&msg,sizeof(msg));
	vfs_chan_unlock(node);
	if(res < 0) {
		vfs_req_free(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);

	/* release resouces before memcpy */
	res = req->count;
	data = req->data;
	vfs_req_free(req);
	if(data && res > 0) {
		thread_addHeapAlloc(data);
		memcpy(buffer,data,res);
		thread_remHeapAlloc(data);
		cache_free(data);
	}
	return res;
}

ssize_t vfs_drv_write(pid_t pid,file_t file,sVFSNode *node,USER const void *buffer,off_t offset,
		size_t count) {
	sRequest *req;
	ssize_t res;
	sArgsMsg msg;

	/* if the driver doesn't implement open, its an error */
	if(!vfs_server_supports(node->parent,DRV_WRITE))
		return ERR_UNSUPPORTED_OP;

	/* get request */
	vfs_chan_lock(node);
	req = vfs_req_get(node,NULL,0);
	if(!req) {
		vfs_chan_unlock(node);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* send msg to driver */
	msg.arg1 = offset;
	msg.arg2 = count;
	res = vfs_sendMsg(pid,file,MSG_DRV_WRITE,&msg,sizeof(msg));
	if(res < 0) {
		vfs_req_free(req);
		vfs_chan_unlock(node);
		return res;
	}
	/* now send data */
	res = vfs_sendMsg(pid,file,MSG_DRV_WRITE,buffer,count);
	vfs_chan_unlock(node);
	if(res < 0) {
		vfs_req_free(req);
		return res;
	}

	/* wait for response */
	vfs_drv_wait(req);
	res = req->count;
	vfs_req_free(req);
	return res;
}

void vfs_drv_close(pid_t pid,file_t file,sVFSNode *node) {
	/* if the driver doesn't implement close, stop here */
	if(!vfs_server_supports(node->parent,DRV_CLOSE))
		return;

	vfs_chan_lock(node);
	vfs_sendMsg(pid,file,MSG_DRV_CLOSE,NULL,0);
	vfs_chan_unlock(node);
}

static void vfs_drv_wait(sRequest *req) {
	/* repeat until it succeeded or the driver died */
	volatile sRequest *r = req;
	do
		vfs_req_waitForReply(req,false);
	while((ssize_t)r->count == ERR_INTERRUPTED);
}

static void vfs_drv_openReqHandler(sVFSNode *node,USER const void *data,size_t size) {
	UNUSED(size);
	sMsg *rmsg = (sMsg*)data;
	ulong res = rmsg->args.arg1;
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = res;
		/* the thread can continue now */
		ev_wakeupThread(req->thread,EV_REQ_REPLY);
		vfs_req_remove(req);
	}
}

static void vfs_drv_readReqHandler(sVFSNode *node,USER const void *data,size_t size) {
	UNUSED(size);
	/* find the request for the node */
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		sVFSNode *drv = node->parent;
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			ulong res = rmsg->args.arg1;
			ulong readable = rmsg->args.arg2;
			/* an error? */
			if((long)res <= 0) {
				vfs_server_setReadable(drv,readable);
				req->count = res;
				req->state = REQ_STATE_FINISHED;
				ev_wakeupThread(req->thread,EV_REQ_REPLY);
				vfs_req_remove(req);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			/* set whether data is readable; do this here because a thread-switch may cause
			 * the driver to set that data is readable although arg2 was 0 here (= no data) */
			vfs_server_setReadable(drv,readable);
			req->count = MIN(req->dsize,res);
			req->state = REQ_STATE_WAIT_DATA;
			vfs_req_release(req);
		}
		else if(req->state == REQ_STATE_WAIT_DATA) {
			/* ok, it's the data */
			if(data) {
				/* map the buffer we have to copy it to */
				req->data = cache_alloc(req->count);
				if(req->data) {
					thread_addHeapAlloc(req->data);
					memcpy(req->data,data,req->count);
					thread_remHeapAlloc(req->data);
				}
			}
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			ev_wakeupThread(req->thread,EV_REQ_REPLY);
			vfs_req_remove(req);
		}
	}
}

static void vfs_drv_writeReqHandler(sVFSNode *node,USER const void *data,size_t size) {
	UNUSED(size);
	sMsg *rmsg = (sMsg*)data;
	ulong res = rmsg->args.arg1;
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = res;
		/* the thread can continue now */
		ev_wakeupThread(req->thread,EV_REQ_REPLY);
		vfs_req_remove(req);
	}
}
