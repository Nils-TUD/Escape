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
#include <vfs.h>
#include <vfsnode.h>
#include <vfsreal.h>
#include <util.h>
#include <string.h>
#include <errors.h>

#include <fsinterface.h>

#define REQUEST_COUNT 1024

/* the header for all default-messages */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgHeader;

/* an entry in the request-list */
typedef struct {
	tTid tid;
	bool finished;
	tInodeNo inodeNo;
	sMsgDataFSReadResp *readResp;
	u8 *buffer;
	u32 count;
} sRequest;

/* read-msg */
typedef struct {
	sMsgHeader header;
	sMsgDataFSReadReq data;
} __attribute__((packed)) sMsgReadReq;
/* close-msg */
typedef struct {
	sMsgHeader header;
	sMsgDataFSCloseReq data;
} __attribute__((packed)) sMsgCloseReq;

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
 * Searches for the request of the given thread
 *
 * @param tid the thread-id
 * @return the request or NULL
 */
static sRequest *vfsr_getRequestByPid(tTid tid);

/* the messages we'll send */
static sMsgReadReq msgRead = {
	.header = {
		.id = MSG_FS_READ,
		.length = sizeof(sMsgDataFSReadReq)
	},
	.data = {
		.inodeNo = 0,
		.offset = 0,
		.count = 0
	}
};
static sMsgCloseReq msgClose = {
	.header = {
		.id = MSG_FS_CLOSE,
		.length = sizeof(sMsgDataFSCloseReq)
	},
	.data = {
		.inodeNo = 0
	}
};

/* the vfs-service-file */
/*static sSLList *requests = NULL;*/
static sRequest requests[REQUEST_COUNT];
static tFileNo fsServiceFile = -1;
static bool gotMsg = false;

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

void vfsr_setGotMsg(void) {
	gotMsg = true;
}

void vfsr_checkForMsgs(void) {
	sMsgHeader header;
	sRequest *req;
	if(!gotMsg)
		return;

	/* ok, let's see what we've got.. */
	while(vfs_readFile(KERNEL_TID,fsServiceFile,(u8*)&header,sizeof(sMsgHeader)) > 0) {
		switch(header.id) {
			case MSG_FS_OPEN_RESP: {
				/* read data */
				sMsgDataFSOpenResp data;
				vfs_readFile(KERNEL_TID,fsServiceFile,(u8*)&data,sizeof(sMsgDataFSOpenResp));

				/* find the request for the tid */
				req = vfsr_getRequestByPid(data.tid);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->inodeNo = data.inodeNo;
					/* the thread can continue now */
					thread_wakeup(data.tid,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_STAT_RESP: {
				/* read data */
				sMsgDataFSStatResp *res = (sMsgDataFSStatResp*)kheap_alloc(sizeof(sMsgDataFSStatResp));
				/* TODO better way? */
				if(res == NULL)
					util_panic("Out of kernel-heapspace");

				vfs_readFile(KERNEL_TID,fsServiceFile,(u8*)res,sizeof(sMsgDataFSStatResp));

				/* find the request for the tid */
				req = vfsr_getRequestByPid(res->tid);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->inodeNo = res->error;
					req->buffer = (u8*)res;
					/* the thread can continue now */
					thread_wakeup(res->tid,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_READ_RESP: {
				/* read data */
				sMsgDataFSReadResp *res = (sMsgDataFSReadResp*)kheap_alloc(header.length);
				/* TODO better way? */
				if(res == NULL)
					util_panic("Out of kernel-heapspace");

				vfs_readFile(KERNEL_TID,fsServiceFile,(u8*)res,header.length);

				/* find the request for the tid */
				req = vfsr_getRequestByPid(res->tid);
				if(req != NULL) {
					/* remove request */
					req->finished = true;
					req->count = res->count;
					req->readResp = res;
					/* the thread can continue now */
					thread_wakeup(res->tid,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_WRITE_RESP: {
				/* read data */
				sMsgDataFSWriteResp data;
				vfs_readFile(KERNEL_TID,fsServiceFile,(u8*)&data,sizeof(sMsgDataFSWriteResp));

				/* find the request for the tid */
				req = vfsr_getRequestByPid(data.tid);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					req->finished = true;
					req->count = data.count;
					/* the thread can continue now */
					thread_wakeup(data.tid,EV_RECEIVED_MSG);
				}
			}
			break;
		}
	}

	gotMsg = false;
}

s32 vfsr_openFile(tTid tid,u8 flags,const char *path) {
	sMsgDataFSOpenReq *data;
	sMsgHeader *msg;
	s32 res;
	u32 pathLen = strlen(path);
	u32 msgLen;
	sRequest *req;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* allocate mem */
	msgLen = sizeof(sMsgDataFSOpenReq) + (pathLen + 1) * sizeof(char);
	msg = (sMsgHeader*)kheap_alloc(sizeof(sMsgHeader) + msgLen);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* copy data */
	msg->id = MSG_FS_OPEN;
	msg->length = msgLen;
	data = (sMsgDataFSOpenReq*)(msg + 1);
	data->flags = flags;
	data->tid = tid;
	memcpy(data->path,path,pathLen + 1);

	/* write message to fs */
	res = vfs_writeFile(KERNEL_TID,fsServiceFile,(u8*)msg,sizeof(sMsgHeader) + msgLen);
	if(res < 0) {
		kheap_free(msg);
		return res;
	}

	/* enqueue request and wait for a reply of the fs */
	kheap_free(msg);
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if(req->inodeNo < 0) {
		tInodeNo no = req->inodeNo;
		req->tid = INVALID_TID;
		return no;
	}
	/* now open the file */
	res = vfs_openFile(tid,flags,req->inodeNo);
	req->tid = INVALID_TID;
	return res;
}

s32 vfsr_getFileInfo(tTid tid,const char *path,sFileInfo *info) {
	sMsgDataFSStatReq *data;
	sMsgHeader *msg;
	s32 res;
	u32 pathLen = strlen(path);
	u32 msgLen;
	sRequest *req;
	sMsgDataFSStatResp *resp;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* allocate mem */
	msgLen = sizeof(sMsgDataFSStatReq) + (pathLen + 1) * sizeof(char);
	msg = (sMsgHeader*)kheap_alloc(sizeof(sMsgHeader) + msgLen);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* copy data */
	msg->id = MSG_FS_STAT;
	msg->length = msgLen;
	data = (sMsgDataFSStatReq*)(msg + 1);
	data->tid = tid;
	memcpy(data->path,path,pathLen + 1);

	/* write message to fs */
	res = vfs_writeFile(KERNEL_TID,fsServiceFile,(u8*)msg,sizeof(sMsgHeader) + msgLen);
	if(res < 0) {
		kheap_free(msg);
		return res;
	}

	/* enqueue request and wait for a reply of the fs */
	kheap_free(msg);
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	/* error? */
	if(req->inodeNo < 0) {
		tInodeNo no = req->inodeNo;
		req->tid = INVALID_TID;
		return no;
	}

	/* copy to info-struct */
	resp = (sMsgDataFSStatResp*)req->buffer;
	memcpy((void*)info,(void*)&(resp->info),sizeof(sFileInfo));
	kheap_free(resp);
	req->tid = INVALID_TID;
	return 0;
}

s32 vfsr_readFile(tTid tid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* build msg */
	msgRead.data.tid = tid;
	msgRead.data.inodeNo = inodeNo;
	msgRead.data.offset = offset;
	msgRead.data.count = count;

	/* write message to fs */
	res = vfs_writeFile(KERNEL_TID,fsServiceFile,(u8*)&msgRead,sizeof(sMsgReadReq));
	if(res < 0)
		return res;
	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	req->buffer = buffer;
	vfsr_waitForReply(tid,req);

	if(req->readResp != NULL) {
		/* copy from temp-buffer to process */
		memcpy(req->buffer,req->readResp->data,req->count);
		kheap_free(req->readResp);
	}

	res = req->count;
	req->tid = INVALID_TID;
	return res;
}

s32 vfsr_writeFile(tTid tid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;
	u32 msgLen;
	sMsgHeader *msg;
	sMsgDataFSWriteReq *data;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* build msg */
	msgLen = sizeof(sMsgHeader) + sizeof(sMsgDataFSWriteReq) + count;
	msg = (sMsgHeader*)kheap_alloc(msgLen);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	msg->id = MSG_FS_WRITE;
	msg->length = sizeof(sMsgDataFSWriteReq) + count;
	data = (sMsgDataFSWriteReq*)(msg + 1);
	data->inodeNo = inodeNo;
	data->count = count;
	data->offset = offset;
	data->tid = tid;
	memcpy(data->data,buffer,count);

	/* write message to fs */
	res = vfs_writeFile(KERNEL_TID,fsServiceFile,(u8*)msg,msgLen);
	if(res < 0) {
		kheap_free(msg);
		return res;
	}

	/* enqueue request and wait for a reply of the fs */
	kheap_free(msg);
	req = vfsr_addRequest(tid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(tid,req);

	req->tid = INVALID_TID;
	res = req->count;
	return res;
}

void vfsr_closeFile(tInodeNo inodeNo) {
	if(fsServiceFile < 0)
		return;

	/* write message to fs */
	msgClose.data.inodeNo = inodeNo;
	vfs_writeFile(KERNEL_TID,fsServiceFile,(u8*)&msgClose,sizeof(sMsgCloseReq));
	/* no response necessary, therefore no wait, too */
}

static void vfsr_waitForReply(tTid tid,sRequest *req) {
	while(!req->finished) {
		thread_wait(tid,EV_RECEIVED_MSG);
		thread_switch();
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
	req->inodeNo = 0;
	req->count = 0;
	req->buffer = NULL;
	req->readResp = NULL;
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
