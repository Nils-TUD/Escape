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
#include <util.h>
#include <string.h>
#include <errors.h>

#include <fsinterface.h>
#include <messages.h>

#define REQUEST_COUNT 1024

/* an entry in the request-list */
typedef struct {
	tTid tid;
	bool finished;
	u32 val1;
	u32 val2;
	void *data;
	u32 count;
} sRequest;

/**
 * Waits for a reply
 *
 * @param tid the thread to block
 * @param req the request
 */
static void vfsr_waitForReply(tTid tid,sRequest *req);

/**
 * Adds a request for the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL if not enough mem
 */
static sRequest *vfsr_addRequest(tTid tid);

/**
 * Marks the given request as finished
 *
 * @param r the request
 */
static void vfsr_remRequest(sRequest *r);

/**
 * Searches for the request of the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL
 */
static sRequest *vfsr_getRequestByPid(tTid tid);

/**
 * KEvent-callback
 */
static void vfsr_gotMsg(u32 param);

/* the vfs-service-file */
static u32 reqCount = 0;
static sRequest requests[REQUEST_COUNT];
static tFileNo fsServiceFile = -1;
static bool kevWait = false;
static bool gotMsg = false;
static sMsg msg;

void vfsr_setFSService(tVFSNodeNo nodeNo) {
	fsServiceFile = vfs_openFileForKernel(KERNEL_TID,nodeNo);
	if(fsServiceFile >= 0) {
		u32 i;
		sRequest *req = requests;
		for(i = 0; i < REQUEST_COUNT; i++) {
			req->tid = INVALID_TID;
			req++;
		}
	}
}

static void vfsr_gotMsg(u32 param) {
	UNUSED(param);
	kevWait = false;
	gotMsg = true;
}

void vfsr_checkForMsgs(void) {
	sRequest *req;
	tMsgId id;
	if(!gotMsg)
		return;

	/* ok, let's see what we've got.. */
	while(vfs_receiveMsg(KERNEL_TID,fsServiceFile,&id,(u8*)&msg,MAX_MSG_SIZE) > 0) {
		switch(id) {
			case MSG_FS_OPEN_RESP: {
				/* find the request for the tid */
				req = vfsr_getRequestByPid((tTid)msg.args.arg1);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->val1 = msg.args.arg2;
					/* the thread can continue now */
					thread_wakeup((tTid)msg.args.arg1,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_STAT_RESP: {
				/* find the request for the tid */
				req = vfsr_getRequestByPid((tTid)msg.data.arg1);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->val1 = msg.data.arg2;
					req->data = (void*)kheap_alloc(sizeof(sFileInfo));
					if(req->data != NULL)
						memcpy(req->data,msg.data.d,sizeof(sFileInfo));
					/* the thread can continue now */
					thread_wakeup((tTid)msg.data.arg1,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_READ_RESP: {
				/* find the request for the tid */
				req = vfsr_getRequestByPid((tTid)msg.data.arg1);
				if(req != NULL) {
					/* remove request */
					req->finished = true;
					req->count = msg.data.arg2;
					req->data = (void*)kheap_alloc(req->count);
					if(req->data != NULL)
						memcpy(req->data,msg.data.d,req->count);
					/* the thread can continue now */
					thread_wakeup((tTid)msg.data.arg1,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_WRITE_RESP: {
				/* find the request for the tid */
				req = vfsr_getRequestByPid((tTid)msg.args.arg1);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->count = msg.args.arg2;
					/* the thread can continue now */
					thread_wakeup((tTid)msg.args.arg1,EV_RECEIVED_MSG);
				}
			}
			break;
		}
	}

	gotMsg = false;
}

s32 vfsr_openFile(tTid tid,u8 flags,const char *path) {
	s32 res;
	u32 pathLen = strlen(path);
	sRequest *req;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;
	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.str.arg1 = tid;
	msg.str.arg2 = flags;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(KERNEL_TID,fsServiceFile,MSG_FS_OPEN,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if(req->val1 < 0) {
		tInodeNo no = req->val1;
		vfsr_remRequest(req);
		return no;
	}
	/* now open the file */
	res = vfs_openFile(tid,flags,req->val1);
	vfsr_remRequest(req);
	return res;
}

s32 vfsr_getFileInfo(tTid tid,const char *path,sFileInfo *info) {
	s32 res;
	u32 pathLen = strlen(path);
	sRequest *req;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;
	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.str.arg1 = tid;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(KERNEL_TID,fsServiceFile,MSG_FS_STAT,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	/* error? */
	if(req->val1 < 0) {
		tInodeNo no = req->val1;
		vfsr_remRequest(req);
		return no;
	}

	/* copy to info-struct */
	if(req->data) {
		memcpy((void*)info,req->data,sizeof(sFileInfo));
		kheap_free(req->data);
	}
	vfsr_remRequest(req);
	return 0;
}

s32 vfsr_readFile(tTid tid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* send msg to fs */
	msg.args.arg1 = tid;
	msg.args.arg2 = inodeNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(KERNEL_TID,fsServiceFile,MSG_FS_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	/* copy from temp-buffer to process */
	if(req->data != NULL) {
		memcpy(buffer,req->data,req->count);
		kheap_free(req->data);
	}

	res = req->count;
	vfsr_remRequest(req);
	return res;
}

s32 vfsr_writeFile(tTid tid,tInodeNo inodeNo,const u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;
	if(count > sizeof(msg.data.d))
		return ERR_INVALID_SYSC_ARGS;

	/* send msg to fs */
	msg.data.arg1 = tid;
	msg.data.arg2 = inodeNo;
	msg.data.arg3 = offset;
	msg.data.arg4 = count;
	memcpy(msg.data.d,buffer,count);
	res = vfs_sendMsg(KERNEL_TID,fsServiceFile,MSG_FS_WRITE,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	res = req->count;
	vfsr_remRequest(req);
	return res;
}

void vfsr_closeFile(tInodeNo inodeNo) {
	if(fsServiceFile < 0)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	vfs_sendMsg(KERNEL_TID,fsServiceFile,MSG_FS_CLOSE,(u8*)&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */
}

static void vfsr_waitForReply(tTid tid,sRequest *req) {
	/* we want to get notified if a message arrives */
	if(!kevWait) {
		kev_add(KEV_VFS_REAL,vfsr_gotMsg);
		kevWait = true;
	}

	while(!req->finished) {
		thread_wait(tid,EV_RECEIVED_MSG);
		thread_switchInKernel();
	}
}

static void vfsr_remRequest(sRequest *r) {
	r->tid = INVALID_TID;
	reqCount--;
	/* ensure that we continue waiting if there is something to wait for */
	if(!kevWait && reqCount > 0) {
		kev_add(KEV_VFS_REAL,vfsr_gotMsg);
		kevWait = true;
	}
}

static sRequest *vfsr_addRequest(tTid tid) {
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
	req->finished = false;
	req->val1 = 0;
	req->val2 = 0;
	req->data = NULL;
	req->count = 0;
	reqCount++;
	return req;
}

static sRequest *vfsr_getRequestByPid(tTid tid) {
	u32 i;
	sRequest *req = requests;
	for(i = 0; i < REQUEST_COUNT; i++) {
		if(req->tid == tid)
			return req;
		req++;
	}
	return NULL;
}
