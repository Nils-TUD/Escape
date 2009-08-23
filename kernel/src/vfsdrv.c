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

#define DRIVER_MAP_SIZE		64

typedef struct {
	tVFSNodeNo nodeNo;
	tFileNo file;
} sDriver;

static void vfsdrv_openReqHandler(const u8 *data,u32 size);
static void vfsdrv_readReqHandler(const u8 *data,u32 size);
static void vfsdrv_writeReqHandler(const u8 *data,u32 size);
static void vfsdrv_ioctlReqHandler(const u8 *data,u32 size);
static sDriver *vfsdrv_get(sVFSNode *node,bool add,s32 *err);
static void vfsdrv_remove(sVFSNode *node);

static sMsg msg;
static sSLList *driver[DRIVER_MAP_SIZE] = {NULL};

void vfsdrv_init(void) {
	vfsreq_setHandler(MSG_DRV_OPEN_RESP,vfsdrv_openReqHandler);
	vfsreq_setHandler(MSG_DRV_READ_RESP,vfsdrv_readReqHandler);
	vfsreq_setHandler(MSG_DRV_WRITE_RESP,vfsdrv_writeReqHandler);
	vfsreq_setHandler(MSG_DRV_IOCTL_RESP,vfsdrv_ioctlReqHandler);
}

s32 vfsdrv_register(sVFSNode *node) {
	s32 res;
	vfsdrv_get(node,true,&res);
	return res;
}

void vfsdrv_unregister(sVFSNode *node) {
	vfsdrv_remove(node);
}

s32 vfsdrv_open(tTid tid,sVFSNode *node,u32 flags) {
	s32 res;
	sDriver *drv;
	sRequest *req;

	drv = vfsdrv_get(node->parent,false,&res);
	if(res < 0)
		return res;

	/* send msg to fs */
	msg.args.arg1 = tid;
	msg.args.arg2 = flags;
	res = vfs_sendMsg(KERNEL_TID,drv->file,MSG_DRV_OPEN,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = (s32)req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_read(tTid tid,sVFSNode *node,void *buffer,u32 offset,u32 count) {
	sRequest *req;
	sDriver *drv;
	s32 res;

	drv = vfsdrv_get(node->parent,false,&res);
	if(res < 0)
		return res;

	/* wait until data is readable */
	while(node->parent->data.service.isEmpty) {
		thread_wait(tid,EV_DATA_READABLE);
		thread_switchInKernel();
	}

	/* send msg to fs */
	msg.args.arg1 = tid;
	msg.args.arg2 = offset;
	msg.args.arg3 = count;
	res = vfs_sendMsg(KERNEL_TID,drv->file,MSG_DRV_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* copy from temp-buffer to process */
	if(req->data != NULL) {
		memcpy(buffer,req->data,req->count);
		kheap_free(req->data);
	}

	res = req->count;
	/* store wether there is more data readable */
	node->parent->data.service.isEmpty = !req->val1;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_write(tTid tid,sVFSNode *node,const void *buffer,u32 offset,u32 count) {
	sRequest *req;
	sDriver *drv;
	s32 res;

	drv = vfsdrv_get(node->parent,false,&res);
	if(res < 0)
		return res;
	/* TODO */
	if(count > sizeof(msg.data.d))
		util_panic("Improve that!!");

	/* send msg to fs */
	msg.data.arg1 = tid;
	msg.data.arg2 = offset;
	msg.data.arg3 = count;
	memcpy(msg.data.d,buffer,count);
	res = vfs_sendMsg(KERNEL_TID,drv->file,MSG_DRV_WRITE,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsdrv_ioctl(tTid tid,sVFSNode *node,u32 cmd,void *data,u32 size) {
	sRequest *req;
	sDriver *drv;
	s32 res;

	drv = vfsdrv_get(node->parent,false,&res);
	if(res < 0)
		return res;
	if(data != NULL && size > sizeof(msg.data.d))
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.data.arg1 = tid;
	msg.data.arg2 = cmd;
	msg.data.arg3 = data != NULL ? size : 0;
	if(data != NULL)
		memcpy(msg.data.d,data,size);
	res = vfs_sendMsg(KERNEL_TID,drv->file,MSG_DRV_IOCTL,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
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

void vfsdrv_close(tTid tid,sVFSNode *node) {
	sDriver *drv;
	s32 res;

	drv = vfsdrv_get(node->parent,false,&res);
	if(res < 0)
		return;

	/* send msg */
	msg.args.arg1 = tid;
	vfs_sendMsg(KERNEL_TID,drv->file,MSG_DRV_CLOSE,(u8*)&msg,sizeof(msg.args));
}

static void vfsdrv_openReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->args.arg1);
	if(req != NULL) {
		/* remove request and give him the result */
		req->finished = true;
		req->count = (u32)rmsg->args.arg2;
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->args.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsdrv_readReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->data.arg1);
	if(req != NULL) {
		/* remove request */
		req->finished = true;
		req->val1 = rmsg->data.arg3;
		req->count = rmsg->data.arg2;
		req->data = (void*)kheap_alloc(req->count);
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,req->count);
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->data.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsdrv_writeReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->args.arg1);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->count = rmsg->args.arg2;
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->args.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsdrv_ioctlReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->data.arg1);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->count = rmsg->data.arg2;
		req->data = NULL;
		if(rmsg->data.arg3 > 0) {
			req->count = rmsg->data.arg3;
			req->data = (void*)kheap_alloc(req->count);
			if(req->data != NULL)
				memcpy(req->data,rmsg->data.d,req->count);
		}
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->data.arg1,EV_RECEIVED_MSG);
	}
}

static sDriver *vfsdrv_get(sVFSNode *node,bool add,s32 *res) {
	sSLNode *n;
	tVFSNodeNo no = vfsn_getNodeNo(node);
	sSLList *list = driver[no % DRIVER_MAP_SIZE];
	s32 err = ERR_NOT_ENOUGH_MEM;

	*res = 0;
	if(list == NULL) {
		if(add) {
			list = driver[no % DRIVER_MAP_SIZE] = sll_create();
			if(list == NULL)
				goto error;
		}
		else {
			err = ERR_INVALID_NODENO;
			goto error;
		}
	}

	for(n = sll_begin(list); n != NULL; n = n->next) {
		if(((sDriver*)n->data)->nodeNo == no) {
			if(add) {
				err = ERR_INVALID_NODENO;
				goto error;
			}
			return (sDriver*)n->data;
		}
	}

	if(add) {
		sDriver *drv = (sDriver*)kheap_alloc(sizeof(sDriver));
		if(drv == NULL)
			goto error;
		drv->nodeNo = no;
		drv->file = vfs_openFileForKernel(KERNEL_TID,no);
		if(drv->file < 0) {
			kheap_free(drv);
			goto error;
		}
		if(!sll_append(list,drv)) {
			vfs_closeFile(KERNEL_TID,drv->file);
			kheap_free(drv);
			goto error;
		}
		return drv;
	}

	err = ERR_INVALID_NODENO;
error:
	*res = err;
	return NULL;
}

static void vfsdrv_remove(sVFSNode *node) {
	sDriver *drv;
	sSLNode *n,*p;
	tVFSNodeNo no = vfsn_getNodeNo(node);
	sSLList *list = driver[no % DRIVER_MAP_SIZE];
	if(list == NULL)
		return;

	p = NULL;
	for(n = sll_begin(list); n != NULL; p = n, n = n->next) {
		drv = (sDriver*)n->data;
		if(drv->nodeNo == no) {
			vfs_closeFile(KERNEL_TID,drv->file);
			kheap_free(drv);
			sll_removeNode(list,n,p);
			return;
		}
	}
}
