/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
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

/* the header for all default-messages */
typedef struct {
	/* the message-id */
	u8 id;
	/* the length of the data behind this struct */
	u32 length;
} sMsgHeader;

/* an entry in the request-list */
typedef struct {
	tPid pid;
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
 * @param pid the process to block
 * @param req the request
 */
static void vfsr_waitForReply(tPid pid,sRequest *req);

/**
 * Adds a request for the given process
 *
 * @param pid the process-id
 * @return the request or NULL if not enough mem
 */
static sRequest *vfsr_addRequest(tPid pid);

/**
 * Searches for the request of the given process
 *
 * @param pid the process-id
 * @return the request or NULL
 */
static sRequest *vfsr_getRequestByPid(tPid pid);

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
static sSLList *requests = NULL;
static tFileNo fsServiceFile = -1;
static bool gotMsg = false;

void vfsr_setFSService(tVFSNodeNo nodeNo) {
	fsServiceFile = vfs_openFileForKernel(KERNEL_PID,nodeNo);
	if(fsServiceFile >= 0) {
		requests = sll_create();
		/* not enough mem */
		if(requests == NULL) {
			vfs_closeFile(fsServiceFile);
			fsServiceFile = -1;
		}
	}
}

void vfsr_setGotMsg(void) {
	gotMsg = true;
}

void vfsr_checkForMsgs(void) {
	sMsgHeader header;
	sRequest *req;
	if(requests == NULL || !gotMsg || sll_length(requests) == 0)
		return;

	/* ok, let's see what we've got.. */
	while(vfs_readFile(KERNEL_PID,fsServiceFile,(u8*)&header,sizeof(sMsgHeader)) > 0) {
		switch(header.id) {
			case MSG_FS_OPEN_RESP: {
				/* read data */
				sMsgDataFSOpenResp data;
				vfs_readFile(KERNEL_PID,fsServiceFile,(u8*)&data,sizeof(sMsgDataFSOpenResp));

				/* find the request for the pid */
				req = vfsr_getRequestByPid(data.pid);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					sll_removeFirst(requests,req);
					req->finished = true;
					req->inodeNo = data.inodeNo;
					/* the process can continue now */
					proc_wakeup(data.pid,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_READ_RESP: {
				/* read data */
				sMsgDataFSReadResp *res = kheap_alloc(header.length);
				/* TODO better way? */
				if(res == NULL)
					panic("Out of kernel-heapspace");

				vfs_readFile(KERNEL_PID,fsServiceFile,(u8*)res,header.length);

				/* find the request for the pid */
				req = vfsr_getRequestByPid(res->pid);
				if(req != NULL) {
					/* remove request */
					sll_removeFirst(requests,req);
					req->finished = true;
					req->count = res->count;
					req->readResp = res;
					/* the process can continue now */
					proc_wakeup(res->pid,EV_RECEIVED_MSG);
				}
			}
			break;

			case MSG_FS_WRITE_RESP: {
				/* read data */
				sMsgDataFSWriteResp data;
				vfs_readFile(KERNEL_PID,fsServiceFile,(u8*)&data,sizeof(sMsgDataFSWriteResp));

				/* find the request for the pid */
				req = vfsr_getRequestByPid(data.pid);
				if(req != NULL) {
					/* remove request and give him the inode-number */
					sll_removeFirst(requests,req);
					req->finished = true;
					req->count = data.count;
					/* the process can continue now */
					proc_wakeup(data.pid,EV_RECEIVED_MSG);
				}
			}
			break;
		}
	}

	gotMsg = false;
}

s32 vfsr_openFile(tPid pid,u8 flags,char *path) {
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
	msg = kheap_alloc(sizeof(sMsgHeader) + msgLen);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* copy data */
	msg->id = MSG_FS_OPEN;
	msg->length = msgLen;
	data = (sMsgDataFSOpenReq*)(msg + 1);
	data->flags = flags;
	data->pid = pid;
	memcpy(data->path,path,pathLen + 1);

	/* write message to fs */
	res = vfs_writeFile(KERNEL_PID,fsServiceFile,(u8*)msg,sizeof(sMsgHeader) + msgLen);
	if(res < 0) {
		kheap_free(msg);
		return res;
	}

	/* enqueue request and wait for a reply of the fs */
	kheap_free(msg);
	req = vfsr_addRequest(pid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(pid,req);

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if(req->inodeNo < 0) {
		tInodeNo no = req->inodeNo;
		kheap_free(req);
		return no;
	}
	/* now open the file */
	res = vfs_openFile(pid,flags,req->inodeNo);
	kheap_free(req);
	return res;
}

s32 vfsr_readFile(tPid pid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* build msg */
	msgRead.data.pid = pid;
	msgRead.data.inodeNo = inodeNo;
	msgRead.data.offset = offset;
	msgRead.data.count = count;

	/* write message to fs */
	res = vfs_writeFile(KERNEL_PID,fsServiceFile,(u8*)&msgRead,sizeof(sMsgReadReq));
	if(res < 0)
		return res;

	/* enqueue request and wait for a reply of the fs */
	req = vfsr_addRequest(pid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	req->buffer = buffer;
	vfsr_waitForReply(pid,req);

	if(req->readResp != NULL) {
		/* copy from temp-buffer to process */
		memcpy(req->buffer,req->readResp->data,req->count);
		kheap_free(req->readResp);
	}

	res = req->count;
	kheap_free(req);
	return res;
}

s32 vfsr_writeFile(tPid pid,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;
	u32 msgLen;
	sMsgHeader *msg;
	sMsgDataFSWriteReq *data;

	if(fsServiceFile < 0)
		return ERR_FS_NOT_FOUND;

	/* build msg */
	msgLen = sizeof(sMsgHeader) + sizeof(sMsgDataFSWriteReq) + count;
	msg = kheap_alloc(msgLen);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	msg->id = MSG_FS_WRITE;
	msg->length = sizeof(sMsgDataFSWriteReq) + count;
	data = (sMsgDataFSWriteReq*)(msg + 1);
	data->inodeNo = inodeNo;
	data->count = count;
	data->offset = offset;
	data->pid = pid;
	memcpy(data->data,buffer,count);

	/* write message to fs */
	res = vfs_writeFile(KERNEL_PID,fsServiceFile,(u8*)msg,msgLen);
	if(res < 0) {
		kheap_free(msg);
		return res;
	}

	/* enqueue request and wait for a reply of the fs */
	kheap_free(msg);
	req = vfsr_addRequest(pid);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;
	vfsr_waitForReply(pid,req);

	res = req->count;
	kheap_free(req);
	return res;
}

void vfsr_closeFile(tInodeNo inodeNo) {
	if(fsServiceFile < 0)
		return;

	/* write message to fs */
	msgClose.data.inodeNo = inodeNo;
	vfs_writeFile(KERNEL_PID,fsServiceFile,(u8*)&msgClose,sizeof(sMsgCloseReq));
	/* no response necessary, therefore no wait, too */
}

static void vfsr_waitForReply(tPid pid,sRequest *req) {
	while(!req->finished) {
		proc_wait(pid,EV_RECEIVED_MSG);
		proc_switch();
	}
}

static sRequest *vfsr_addRequest(tPid pid) {
	sRequest *req = kheap_alloc(sizeof(sRequest));
	if(req == NULL)
		return NULL;

	req->pid = pid;
	req->finished = false;
	req->inodeNo = 0;
	req->count = 0;
	req->buffer = NULL;
	req->readResp = NULL;
	sll_append(requests,req);
	return req;
}

static sRequest *vfsr_getRequestByPid(tPid pid) {
	sSLNode *n;
	sRequest *req;
	for(n = sll_begin(requests); n != NULL; n = n->next) {
		req = (sRequest*)n->data;
		if(req->pid == pid)
			return req;
	}
	return NULL;
}
