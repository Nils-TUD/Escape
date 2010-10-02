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
#include <sys/task/proc.h>
#include <sys/task/event.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/request.h>
#include <sys/vfs/real.h>
#include <sys/vfs/channel.h>
#include <sys/util.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>

#define FS_PATH				"/dev/fs"

typedef struct {
	u8 active;
	tFileNo file;
	sVFSNode *node;
} sFSChan;

/* for istat() and stat() */
static s32 vfsr_doStat(tPid pid,const char *path,tInodeNo ino,tDevNo devNo,sFileInfo *info);
/* The request-handler for sending a path and receiving a result */
static s32 vfsr_pathReqHandler(tPid pid,const char *path1,const char *path2,u32 arg1,u32 cmd);
/* waits for a response */
static void vfsr_wait(sRequest *req);

/* The response-handler for the different message-ids */
static void vfsr_openRespHandler(sVFSNode *node,const u8 *data,u32 size);
static void vfsr_readRespHandler(sVFSNode *node,const u8 *data,u32 size);
static void vfsr_statRespHandler(sVFSNode *node,const u8 *data,u32 size);
static void vfsr_defRespHandler(sVFSNode *node,const u8 *data,u32 size);

/* for requesting and releasing a file for communication */
static tFileNo vfsr_requestFile(tPid pid,sVFSNode **node);
static void vfsr_releaseFile(tPid pid,tFileNo file);

static sMsg msg;

void vfsr_init(void) {
	vfsreq_setHandler(MSG_FS_OPEN_RESP,vfsr_openRespHandler);
	vfsreq_setHandler(MSG_FS_READ_RESP,vfsr_readRespHandler);
	vfsreq_setHandler(MSG_FS_STAT_RESP,vfsr_statRespHandler);
	vfsreq_setHandler(MSG_FS_ISTAT_RESP,vfsr_statRespHandler);
	vfsreq_setHandler(MSG_FS_WRITE_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_LINK_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_UNLINK_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_MKDIR_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_RMDIR_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_MOUNT_RESP,vfsr_defRespHandler);
	vfsreq_setHandler(MSG_FS_UNMOUNT_RESP,vfsr_defRespHandler);
}

void vfsr_removeProc(tPid pid) {
	sProc *p = proc_getByPid(pid);
	if(p->fsChans) {
		sSLNode *n;
		for(n = sll_begin(p->fsChans); n != NULL; n = n->next) {
			sFSChan *chan = (sFSChan*)n->data;
			vfs_closeFile(pid,chan->file);
		}
		sll_destroy(p->fsChans,true);
		p->fsChans = NULL;
	}
}

s32 vfsr_openFile(tPid pid,u16 flags,const char *path) {
	s32 res;
	u32 pathLen = strlen(path);
	sVFSNode *node;
	tFileNo fs;
	sRequest *req;

	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
	fs = vfsr_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfsreq_getRequest(node,NULL,0);

	/* send msg to fs */
	msg.str.arg1 = flags;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(pid,fs,MSG_FS_OPEN,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfsr_wait(req);

	/* error? */
	if((s32)req->count < 0) {
		res = req->count;
		goto error;
	}

	/* now open the file */
	res = vfs_openFile(pid,flags,(tInodeNo)req->count,(tDevNo)req->val1);

error:
	vfsreq_remRequest(req);
	vfsr_releaseFile(pid,fs);
	return res;
}

s32 vfsr_openInode(tPid pid,u16 flags,tInodeNo ino,tDevNo dev) {
	/* TODO maybe we should send an open-msg to fs, too, but in a different form? */
	/* open the file */
	return vfs_openFile(pid,flags,ino,dev);
}

s32 vfsr_istat(tPid pid,tInodeNo ino,tDevNo devNo,sFileInfo *info) {
	return vfsr_doStat(pid,NULL,ino,devNo,info);
}

s32 vfsr_stat(tPid pid,const char *path,sFileInfo *info) {
	return vfsr_doStat(pid,path,0,0,info);
}

static s32 vfsr_doStat(tPid pid,const char *path,tInodeNo ino,tDevNo devNo,sFileInfo *info) {
	s32 res;
	u32 pathLen = 0;
	tFileNo fs;
	sRequest *req;
	sVFSNode *node;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return ERR_INVALID_ARGS;
	}
	fs = vfsr_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfsreq_getRequest(node,NULL,0);

	/* send msg to fs */
	if(path) {
		memcpy(msg.str.s1,path,pathLen + 1);
		res = vfs_sendMsg(pid,fs,MSG_FS_STAT,(u8*)&msg,sizeof(msg.str));
	}
	else {
		msg.args.arg1 = ino;
		msg.args.arg2 = devNo;
		res = vfs_sendMsg(pid,fs,MSG_FS_ISTAT,(u8*)&msg,sizeof(msg.args));
	}
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfsr_wait(req);

	/* error? */
	if((s32)req->count < 0) {
		res = req->count;
		goto error;
	}

	/* copy to info-struct */
	if(req->data) {
		memcpy((void*)info,req->data,sizeof(sFileInfo));
		kheap_free(req->data);
	}
	res = 0;

error:
	vfsreq_remRequest(req);
	vfsr_releaseFile(pid,fs);
	return res;
}

s32 vfsr_readFile(tPid pid,tInodeNo inodeNo,tDevNo devNo,u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;
	void *data;
	sVFSNode *node;
	tFileNo fs = vfsr_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfsreq_getRequest(node,NULL,0);

	/* send msg to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0) {
		vfsreq_remRequest(req);
		vfsr_releaseFile(pid,fs);
		return res;
	}

	/* wait for a reply */
	vfsr_wait(req);

	res = req->count;
	data = req->data;
	/* Better release the request before the memcpy so that it can be reused. Because memcpy might
	 * cause a page-fault which leads to swapping -> thread-switch. */
	vfsreq_remRequest(req);
	vfsr_releaseFile(pid,fs);
	if(data) {
		memcpy(buffer,data,req->count);
		kheap_free(data);
	}
	return res;
}

s32 vfsr_writeFile(tPid pid,tInodeNo inodeNo,tDevNo devNo,const u8 *buffer,u32 offset,u32 count) {
	sRequest *req;
	s32 res;
	sVFSNode *node;
	tFileNo fs = vfsr_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfsreq_getRequest(node,NULL,0);

	/* send msg first */
	msg.data.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		goto error;
	/* now send data */
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,buffer,count);
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfsr_wait(req);
	res = req->count;

error:
	vfsreq_remRequest(req);
	vfsr_releaseFile(pid,fs);
	return res;
}

s32 vfsr_link(tPid pid,const char *oldPath,const char *newPath) {
	return vfsr_pathReqHandler(pid,oldPath,newPath,0,MSG_FS_LINK);
}

s32 vfsr_unlink(tPid pid,const char *path) {
	return vfsr_pathReqHandler(pid,path,NULL,0,MSG_FS_UNLINK);
}

s32 vfsr_mkdir(tPid pid,const char *path) {
	return vfsr_pathReqHandler(pid,path,NULL,0,MSG_FS_MKDIR);
}

s32 vfsr_rmdir(tPid pid,const char *path) {
	return vfsr_pathReqHandler(pid,path,NULL,0,MSG_FS_RMDIR);
}

s32 vfsr_mount(tPid pid,const char *device,const char *path,u16 type) {
	return vfsr_pathReqHandler(pid,device,path,type,MSG_FS_MOUNT);
}

s32 vfsr_unmount(tPid pid,const char *path) {
	return vfsr_pathReqHandler(pid,path,NULL,0,MSG_FS_UNMOUNT);
}

s32 vfsr_sync(tPid pid) {
	s32 res;
	tFileNo fs = vfsr_requestFile(pid,NULL);
	if(fs < 0)
		return fs;
	res = vfs_sendMsg(pid,fs,MSG_FS_SYNC,(u8*)&msg,sizeof(msg.args));
	vfsr_releaseFile(pid,fs);
	return res;
}

void vfsr_closeFile(tPid pid,tInodeNo inodeNo,tDevNo devNo) {
	tFileNo fs = vfsr_requestFile(pid,NULL);
	if(fs < 0)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	vfs_sendMsg(pid,fs,MSG_FS_CLOSE,(u8*)&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */
	vfsr_releaseFile(pid,fs);
}

static s32 vfsr_pathReqHandler(tPid pid,const char *path1,const char *path2,u32 arg1,u32 cmd) {
	s32 res;
	sRequest *req;
	tFileNo fs;
	sVFSNode *node;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;

	fs = vfsr_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfsreq_getRequest(node,NULL,0);

	/* send msg */
	strcpy(msg.str.s1,path1);
	if(path2)
		strcpy(msg.str.s2,path2);
	msg.str.arg1 = arg1;
	res = vfs_sendMsg(pid,fs,cmd,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfsr_wait(req);
	res = req->count;

error:
	vfsreq_remRequest(req);
	vfsr_releaseFile(pid,fs);
	return res;
}

static void vfsr_wait(sRequest *req) {
	do
		vfsreq_waitForReply(req,false);
	while((s32)req->count == ERR_DRIVER_DIED);
}

static void vfsr_openRespHandler(sVFSNode *node,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfsreq_getRequestByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		req->val1 = rmsg->args.arg2;
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static void vfsr_readRespHandler(sVFSNode *node,const u8 *data,u32 size) {
	/* find the request for the node */
	sRequest *req = vfsreq_getRequestByNode(node);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(!data || size < sizeof(rmsg->args) || (s32)rmsg->args.arg1 <= 0) {
				req->count = 0;
				req->state = REQ_STATE_FINISHED;
				ev_wakeupThread(req->tid,EV_REQ_REPLY);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			req->count = rmsg->args.arg1;
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

static void vfsr_statRespHandler(sVFSNode *node,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->data))
		return;

	/* find the request for the node */
	req = vfsreq_getRequestByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->data.arg1;
		req->data = (void*)kheap_alloc(sizeof(sFileInfo));
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static void vfsr_defRespHandler(sVFSNode *node,const u8 *data,u32 size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfsreq_getRequestByNode(node);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static tFileNo vfsr_requestFile(tPid pid,sVFSNode **node) {
	s32 err;
	sSLNode *n;
	sFSChan *chan;
	sVFSNode *child;
	tInodeNo nodeNo;
	sProc *p = proc_getByPid(pid);
	if(!p->fsChans) {
		p->fsChans = sll_create();
		if(!p->fsChans)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* check if there's a free channel */
	for(n = sll_begin(p->fsChans); n != NULL; n = n->next) {
		chan = (sFSChan*)n->data;
		if(!chan->active) {
			if(node)
				*node = chan->node;
			chan->active = true;
			return chan->file;
		}
	}

	chan = (sFSChan*)kheap_alloc(sizeof(sFSChan));
	if(!chan)
		return ERR_NOT_ENOUGH_MEM;
	chan->active = true;

	/* resolve path to fs */
	err = vfsn_resolvePath(FS_PATH,&nodeNo,NULL,VFS_READ | VFS_WRITE);
	if(err < 0)
		goto errorChan;

	/* create usage-node */
	chan->node = vfsn_getNode(nodeNo);
	child = vfs_chan_create(pid,chan->node);
	if(child == NULL) {
		err = ERR_NOT_ENOUGH_MEM;
		goto errorChan;
	}
	chan->node = child;
	nodeNo = vfsn_getNodeNo(child);

	/* open file */
	err = vfs_openFile(pid,VFS_READ | VFS_WRITE,nodeNo,VFS_DEV_NO);
	if(err < 0)
		goto errorNode;
	chan->file = err;
	if(!sll_append(p->fsChans,chan))
		goto errorClose;
	if(node)
		*node = chan->node;
	return chan->file;

errorClose:
	vfs_closeFile(pid,chan->file);
errorNode:
	vfs_node_destroy(child);
errorChan:
	kheap_free(chan);
	return err;
}

static void vfsr_releaseFile(tPid pid,tFileNo file) {
	sSLNode *n;
	sProc *p = proc_getByPid(pid);
	for(n = sll_begin(p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		if(chan->file == file) {
			chan->active = false;
			return;
		}
	}
}
