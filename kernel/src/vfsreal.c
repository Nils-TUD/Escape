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
#include <vfsreq.h>
#include <vfsreal.h>
#include <util.h>
#include <string.h>
#include <errors.h>

#include <fsinterface.h>
#include <messages.h>

/* The request-handler for the different message-ids */
static void vfsr_openReqHandler(const u8 *data,u32 size);
static void vfsr_readReqHandler(const u8 *data,u32 size);
static void vfsr_statReqHandler(const u8 *data,u32 size);
static void vfsr_writeReqHandler(const u8 *data,u32 size);

static tFileNo fs = -1;
static sMsg msg;

void vfsr_init(tVFSNodeNo fsNode) {
	fs = vfs_openFileForKernel(KERNEL_TID,fsNode);
	if(fs < 0)
		return;
	vfsreq_setHandler(MSG_FS_OPEN_RESP,vfsr_openReqHandler);
	vfsreq_setHandler(MSG_FS_READ_RESP,vfsr_readReqHandler);
	vfsreq_setHandler(MSG_FS_STAT_RESP,vfsr_statReqHandler);
	vfsreq_setHandler(MSG_FS_WRITE_RESP,vfsr_writeReqHandler);
}

s32 vfsr_openFile(tTid tid,u8 flags,const char *path) {
	s32 res;
	u32 pathLen = strlen(path);
	sRequest *req;

	if(fs < 0)
		return ERR_FS_NOT_FOUND;
	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.str.arg1 = tid;
	msg.str.arg2 = flags;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(KERNEL_TID,fs,MSG_FS_OPEN,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsreq_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsreq_waitForReply(tid,req);

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if((s32)req->val1 < 0) {
		tInodeNo no = req->val1;
		vfsreq_remRequest(req);
		return no;
	}
	/* now open the file */
	res = vfs_openFile(tid,flags,req->val1);
	vfsreq_remRequest(req);
	return res;
}

s32 vfsr_getFileInfo(tTid tid,const char *path,sFileInfo *info) {
	s32 res;
	u32 pathLen = strlen(path);
	sRequest *req;

	if(fs < 0)
		return ERR_FS_NOT_FOUND;
	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.str.arg1 = tid;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(KERNEL_TID,fs,MSG_FS_STAT,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsreq_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsreq_waitForReply(tid,req);

	/* error? */
	if((s32)req->val1 < 0) {
		tInodeNo no = req->val1;
		vfsreq_remRequest(req);
		return no;
	}

	/* copy to info-struct */
	if(req->data) {
		memcpy((void*)info,req->data,sizeof(sFileInfo));
		kheap_free(req->data);
	}
	vfsreq_remRequest(req);
	return 0;
}

s32 vfsr_readFile(tTid tid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fs < 0)
		return ERR_FS_NOT_FOUND;

	/* send msg to fs */
	msg.args.arg1 = tid;
	msg.args.arg2 = inodeNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(KERNEL_TID,fs,MSG_FS_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsreq_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsreq_waitForReply(tid,req);

	/* copy from temp-buffer to process */
	if(req->data != NULL) {
		memcpy(buffer,req->data,req->count);
		kheap_free(req->data);
	}

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsr_writeFile(tTid tid,tInodeNo inodeNo,const u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fs < 0)
		return ERR_FS_NOT_FOUND;
	if(count > sizeof(msg.data.d))
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.data.arg1 = tid;
	msg.data.arg2 = inodeNo;
	msg.data.arg3 = offset;
	msg.data.arg4 = count;
	memcpy(msg.data.d,buffer,count);
	res = vfs_sendMsg(KERNEL_TID,fs,MSG_FS_WRITE,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsreq_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsreq_waitForReply(tid,req);

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

void vfsr_closeFile(tInodeNo inodeNo) {
	if(fs < 0)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	vfs_sendMsg(KERNEL_TID,fs,MSG_FS_CLOSE,(u8*)&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */
}

static void vfsr_openReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->args.arg1);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->val1 = rmsg->args.arg2;
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->args.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsr_readReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->data.arg1);
	if(req != NULL) {
		/* remove request */
		req->finished = true;
		req->count = rmsg->data.arg2;
		req->data = (void*)kheap_alloc(req->count);
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,req->count);
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->data.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsr_statReqHandler(const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid((tTid)rmsg->data.arg1);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->val1 = rmsg->data.arg2;
		req->data = (void*)kheap_alloc(sizeof(sFileInfo));
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
		/* the thread can continue now */
		thread_wakeup((tTid)rmsg->data.arg1,EV_RECEIVED_MSG);
	}
}

static void vfsr_writeReqHandler(const u8 *data,u32 size) {
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
