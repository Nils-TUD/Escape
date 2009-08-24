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

#define FS_PATH				"/services/fs"
#define R2V_MAP_SIZE		64

typedef struct {
	tFileNo virtFile;
	tFileNo realFile;
} sReal2Virt;

/* The request-handler for the different message-ids */
static void vfsr_openReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsr_readReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsr_statReqHandler(tTid tid,const u8 *data,u32 size);
static void vfsr_writeReqHandler(tTid tid,const u8 *data,u32 size);
static tFileNo vfsr_create(tTid tid);
static s32 vfsr_add(tFileNo virtFile,tFileNo realFile);
static sReal2Virt *vfsr_get(tFileNo real,s32 *err);
static void vfsr_remove(tTid tid,tFileNo real);
static void vfsr_destroy(tTid tid,tFileNo virt);

static sMsg msg;
static sSLList *real2virt[R2V_MAP_SIZE] = {NULL};

void vfsr_init(void) {
	vfsreq_setHandler(MSG_FS_OPEN_RESP,vfsr_openReqHandler);
	vfsreq_setHandler(MSG_FS_READ_RESP,vfsr_readReqHandler);
	vfsreq_setHandler(MSG_FS_STAT_RESP,vfsr_statReqHandler);
	vfsreq_setHandler(MSG_FS_WRITE_RESP,vfsr_writeReqHandler);
}

s32 vfsr_openFile(tTid tid,u8 flags,const char *path) {
	s32 res;
	u32 pathLen = strlen(path);
	tFileNo virtFile,realFile;
	sRequest *req;

	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;
	virtFile = vfsr_create(tid);
	if(virtFile < 0)
		return virtFile;

	/* send msg to fs */
	msg.str.arg1 = flags;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(tid,virtFile,MSG_FS_OPEN,(u8*)&msg,sizeof(msg.str));
	if(res < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
	if(req == NULL) {
		vfsr_destroy(tid,virtFile);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if((s32)req->val1 < 0) {
		tInodeNo no = req->val1;
		vfsreq_remRequest(req);
		vfsr_destroy(tid,virtFile);
		return no;
	}

	/* now open the file */
	realFile = vfs_openFile(tid,flags,req->val1);
	vfsreq_remRequest(req);
	if((res = vfsr_add(virtFile,realFile)) < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}
	return realFile;
}

s32 vfsr_getFileInfo(tTid tid,const char *path,sFileInfo *info) {
	s32 res;
	u32 pathLen = strlen(path);
	tFileNo virtFile;
	sRequest *req;

	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_SYSC_ARGS;
	virtFile = vfsr_create(tid);
	if(virtFile < 0)
		return virtFile;

	/* send msg to fs */
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(tid,virtFile,MSG_FS_STAT,(u8*)&msg,sizeof(msg.str));
	if(res < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}

	/* wait for a reply */
	req = vfsreq_waitForReply(tid);
	if(req == NULL) {
		vfsr_destroy(tid,virtFile);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* error? */
	vfsr_destroy(tid,virtFile);
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

s32 vfsr_readFile(tTid tid,tFileNo file,tInodeNo inodeNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	sReal2Virt *r2v;
	s32 res;

	r2v = vfsr_get(file,&res);
	if(r2v == NULL)
		return res;

	/* send msg to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = offset;
	msg.args.arg3 = count;
	res = vfs_sendMsg(tid,r2v->virtFile,MSG_FS_READ,(u8*)&msg,sizeof(msg.args));
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
	vfsreq_remRequest(req);
	return res;
}

s32 vfsr_writeFile(tTid tid,tFileNo file,tInodeNo inodeNo,const u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	sReal2Virt *r2v;
	s32 res;

	/* TODO */
	if(count > sizeof(msg.data.d))
		return ERR_INVALID_SYSC_ARGS;
	r2v = vfsr_get(file,&res);
	if(r2v == NULL)
		return res;

	/* send msg to fs */
	msg.data.arg1 = inodeNo;
	msg.data.arg2 = offset;
	msg.data.arg3 = count;
	memcpy(msg.data.d,buffer,count);
	res = vfs_sendMsg(tid,r2v->virtFile,MSG_FS_WRITE,(u8*)&msg,sizeof(msg.data));
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

void vfsr_closeFile(tTid tid,tFileNo file,tInodeNo inodeNo) {
	s32 res;
	sReal2Virt *r2v;
	r2v = vfsr_get(file,&res);
	if(r2v == NULL)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	vfs_sendMsg(tid,r2v->virtFile,MSG_FS_CLOSE,(u8*)&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */

	/* remove the stuff for the communication */
	vfsr_remove(tid,file);
}

static void vfsr_openReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->val1 = rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static void vfsr_readReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request */
		req->finished = true;
		req->count = rmsg->data.arg1;
		req->data = (void*)kheap_alloc(req->count);
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,req->count);
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static void vfsr_statReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->val1 = rmsg->data.arg1;
		req->data = (void*)kheap_alloc(sizeof(sFileInfo));
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static void vfsr_writeReqHandler(tTid tid,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	if(size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->finished = true;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_RECEIVED_MSG);
	}
}

static tFileNo vfsr_create(tTid tid) {
	s32 res;
	tVFSNodeNo nodeNo;

	/* create a virtual node for communication with fs */
	if((res = vfsn_resolvePath(FS_PATH,&nodeNo,true)) != 0)
		return res;
	/* open the file */
	return vfs_openFile(tid,VFS_READ | VFS_WRITE,nodeNo);
}

static s32 vfsr_add(tFileNo virtFile,tFileNo realFile) {
	sReal2Virt *r2v;
	sSLList *list;

	/* determine list */
	list = real2virt[realFile % R2V_MAP_SIZE];
	if(list == NULL) {
		list = real2virt[realFile % R2V_MAP_SIZE] = sll_create();
		if(list == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* create storage for real->virt */
	r2v = (sReal2Virt*)kheap_alloc(sizeof(sReal2Virt));
	if(r2v == NULL)
		return ERR_NOT_ENOUGH_MEM;

	r2v->realFile = realFile;
	r2v->virtFile = virtFile;

	/* append to list */
	if(!sll_append(list,r2v)) {
		kheap_free(r2v);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

static sReal2Virt *vfsr_get(tFileNo real,s32 *err) {
	sSLNode *n;
	sSLList *list = real2virt[real % R2V_MAP_SIZE];
	*err = ERR_INVALID_NODENO;
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		if(((sReal2Virt*)n->data)->realFile == real) {
			*err = 0;
			return (sReal2Virt*)n->data;
		}
	}
	return NULL;
}

static void vfsr_remove(tTid tid,tFileNo real) {
	sReal2Virt *r2v;
	sSLNode *n,*p;
	sSLList *list = real2virt[real % R2V_MAP_SIZE];
	if(list == NULL)
		return;

	p = NULL;
	for(n = sll_begin(list); n != NULL; p = n, n = n->next) {
		r2v = (sReal2Virt*)n->data;
		if(r2v->realFile == real) {
			vfsr_destroy(tid,r2v->virtFile);
			kheap_free(r2v);
			sll_removeNode(list,n,p);
			return;
		}
	}
}

static void vfsr_destroy(tTid tid,tFileNo virt) {
	vfs_closeFile(tid,virt);
}
