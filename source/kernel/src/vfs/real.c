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
#include <sys/task/proc.h>
#include <sys/task/event.h>
#include <sys/mem/cache.h>
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
	uint8_t active;
	file_t file;
	sVFSNode *node;
} sFSChan;

/* for istat() and stat() */
static int vfs_real_doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,sFileInfo *info);
/* The request-handler for sending a path and receiving a result */
static int vfs_real_pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd);
/* waits for a response */
static void vfs_real_wait(sRequest *req);

/* The response-handler for the different message-ids */
static void vfs_real_openRespHandler(sVFSNode *node,const void *data,size_t size);
static void vfs_real_readRespHandler(sVFSNode *node,const void *data,size_t size);
static void vfs_real_statRespHandler(sVFSNode *node,const void *data,size_t size);
static void vfs_real_defRespHandler(sVFSNode *node,const void *data,size_t size);

/* for requesting and releasing a file for communication */
static file_t vfs_real_requestFile(pid_t pid,sVFSNode **node);
static void vfs_real_releaseFile(pid_t pid,file_t file);

static sMsg msg;

void vfs_real_init(void) {
	vfs_req_setHandler(MSG_FS_OPEN_RESP,vfs_real_openRespHandler);
	vfs_req_setHandler(MSG_FS_READ_RESP,vfs_real_readRespHandler);
	vfs_req_setHandler(MSG_FS_STAT_RESP,vfs_real_statRespHandler);
	vfs_req_setHandler(MSG_FS_ISTAT_RESP,vfs_real_statRespHandler);
	vfs_req_setHandler(MSG_FS_WRITE_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_LINK_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_UNLINK_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_MKDIR_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_RMDIR_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_MOUNT_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_UNMOUNT_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_CHMOD_RESP,vfs_real_defRespHandler);
	vfs_req_setHandler(MSG_FS_CHOWN_RESP,vfs_real_defRespHandler);
}

void vfs_real_removeProc(pid_t pid) {
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

file_t vfs_real_openPath(pid_t pid,uint flags,const char *path) {
	const sProc *p = proc_getByPid(pid);
	ssize_t res = ERR_NOT_ENOUGH_MEM;
	size_t pathLen = strlen(path);
	sVFSNode *node;
	file_t fs;
	sRequest *req;

	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
	fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,NULL,0);
	if(!req)
		goto error;

	/* send msg to fs */
	msg.str.arg1 = flags;
	msg.str.arg2 = p->euid;
	msg.str.arg3 = p->egid;
	msg.str.arg4 = p->pid;
	memcpy(msg.str.s1,path,pathLen + 1);
	res = vfs_sendMsg(pid,fs,MSG_FS_OPEN,&msg,sizeof(msg.str));
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfs_real_wait(req);

	/* error? */
	if((ssize_t)req->count < 0) {
		res = req->count;
		goto error;
	}

	/* now open the file */
	res = vfs_openFile(pid,flags,(inode_t)req->count,(dev_t)req->val1);

error:
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	return res;
}

file_t vfs_real_openInode(pid_t pid,uint flags,inode_t ino,dev_t dev) {
	/* TODO maybe we should send an open-msg to fs, too, but in a different form? */
	/* open the file */
	return vfs_openFile(pid,flags,ino,dev);
}

int vfs_real_istat(pid_t pid,inode_t ino,dev_t devNo,sFileInfo *info) {
	return vfs_real_doStat(pid,NULL,ino,devNo,info);
}

int vfs_real_stat(pid_t pid,const char *path,sFileInfo *info) {
	return vfs_real_doStat(pid,path,0,0,info);
}

static int vfs_real_doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,sFileInfo *info) {
	int res = ERR_NOT_ENOUGH_MEM;
	size_t pathLen = 0;
	file_t fs;
	sRequest *req;
	sVFSNode *node;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return ERR_INVALID_ARGS;
	}
	fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,NULL,0);
	if(!req)
		goto error;

	/* send msg to fs */
	if(path) {
		const sProc *p = proc_getByPid(pid);
		msg.str.arg1 = p->euid;
		msg.str.arg2 = p->egid;
		msg.str.arg3 = p->pid;
		memcpy(msg.str.s1,path,pathLen + 1);
		res = vfs_sendMsg(pid,fs,MSG_FS_STAT,&msg,sizeof(msg.str));
	}
	else {
		msg.args.arg1 = ino;
		msg.args.arg2 = devNo;
		res = vfs_sendMsg(pid,fs,MSG_FS_ISTAT,&msg,sizeof(msg.args));
	}
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfs_real_wait(req);

	/* error? */
	if((ssize_t)req->count < 0) {
		res = req->count;
		goto error;
	}

	/* copy to info-struct */
	if(req->data) {
		memcpy((void*)info,req->data,sizeof(sFileInfo));
		cache_free(req->data);
	}
	res = 0;

error:
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	return res;
}

ssize_t vfs_real_read(pid_t pid,inode_t inodeNo,dev_t devNo,void *buffer,off_t offset,size_t count) {
	sRequest *req;
	ssize_t res;
	void *data;
	sVFSNode *node;
	file_t fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,buffer,0);
	if(!req) {
		vfs_real_releaseFile(pid,fs);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* send msg to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_READ,&msg,sizeof(msg.args));
	if(res < 0) {
		vfs_req_free(req);
		vfs_real_releaseFile(pid,fs);
		return res;
	}

	/* wait for a reply */
	vfs_real_wait(req);

	res = req->count;
	data = req->data;
	/* Better release the request before the memcpy so that it can be reused. Because memcpy might
	 * cause a page-fault which leads to swapping -> thread-switch. */
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	if(data) {
		memcpy(buffer,data,res);
		cache_free(data);
	}
	return res;
}

ssize_t vfs_real_write(pid_t pid,inode_t inodeNo,dev_t devNo,const void *buffer,off_t offset,
		size_t count) {
	sRequest *req;
	ssize_t res = ERR_NOT_ENOUGH_MEM;
	sVFSNode *node;
	file_t fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,NULL,0);
	if(!req)
		goto error;

	/* send msg first */
	msg.data.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,&msg,sizeof(msg.data));
	if(res < 0)
		goto error;
	/* now send data */
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,buffer,count);
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfs_real_wait(req);
	res = req->count;

error:
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	return res;
}

int vfs_real_chmod(pid_t pid,const char *path,mode_t mode) {
	return vfs_real_pathReqHandler(pid,path,NULL,mode,MSG_FS_CHMOD);
}

int vfs_real_chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	/* TODO better solution? */
	return vfs_real_pathReqHandler(pid,path,NULL,(uid << 16) | (gid & 0xFFFF),MSG_FS_CHOWN);
}

int vfs_real_link(pid_t pid,const char *oldPath,const char *newPath) {
	return vfs_real_pathReqHandler(pid,oldPath,newPath,0,MSG_FS_LINK);
}

int vfs_real_unlink(pid_t pid,const char *path) {
	return vfs_real_pathReqHandler(pid,path,NULL,0,MSG_FS_UNLINK);
}

int vfs_real_mkdir(pid_t pid,const char *path) {
	return vfs_real_pathReqHandler(pid,path,NULL,0,MSG_FS_MKDIR);
}

int vfs_real_rmdir(pid_t pid,const char *path) {
	return vfs_real_pathReqHandler(pid,path,NULL,0,MSG_FS_RMDIR);
}

int vfs_real_mount(pid_t pid,const char *device,const char *path,uint type) {
	return vfs_real_pathReqHandler(pid,device,path,type,MSG_FS_MOUNT);
}

int vfs_real_unmount(pid_t pid,const char *path) {
	return vfs_real_pathReqHandler(pid,path,NULL,0,MSG_FS_UNMOUNT);
}

int vfs_real_sync(pid_t pid) {
	int res;
	file_t fs = vfs_real_requestFile(pid,NULL);
	if(fs < 0)
		return fs;
	res = vfs_sendMsg(pid,fs,MSG_FS_SYNC,&msg,sizeof(msg.args));
	vfs_real_releaseFile(pid,fs);
	return res;
}

void vfs_real_close(pid_t pid,inode_t inodeNo,dev_t devNo) {
	file_t fs = vfs_real_requestFile(pid,NULL);
	if(fs < 0)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	vfs_sendMsg(pid,fs,MSG_FS_CLOSE,&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */
	vfs_real_releaseFile(pid,fs);
}

static int vfs_real_pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd) {
	int res = ERR_NOT_ENOUGH_MEM;
	const sProc *p = proc_getByPid(pid);
	sRequest *req;
	file_t fs;
	sVFSNode *node;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;

	fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,NULL,0);
	if(!req)
		goto error;

	/* send msg */
	strcpy(msg.str.s1,path1);
	if(path2)
		strcpy(msg.str.s2,path2);
	msg.str.arg1 = arg1;
	msg.str.arg2 = p->euid;
	msg.str.arg3 = p->egid;
	msg.str.arg4 = p->pid;
	res = vfs_sendMsg(pid,fs,cmd,&msg,sizeof(msg.str));
	if(res < 0)
		goto error;

	/* wait for a reply */
	vfs_real_wait(req);
	res = req->count;

error:
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	return res;
}

static void vfs_real_wait(sRequest *req) {
	do
		vfs_req_waitForReply(req,false);
	while((ssize_t)req->count == ERR_DRIVER_DIED);
}

static void vfs_real_openRespHandler(sVFSNode *node,const void *data,size_t size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		req->val1 = rmsg->args.arg2;
		vfs_req_remove(req);
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static void vfs_real_readRespHandler(sVFSNode *node,const void *data,size_t size) {
	/* find the request for the node */
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(!data || size < sizeof(rmsg->args) || (long)rmsg->args.arg1 <= 0) {
				req->count = 0;
				req->state = REQ_STATE_FINISHED;
				req->data = NULL;
				vfs_req_remove(req);
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
				if(IS_SHARED(req->data)) {
					memcpy(req->data,data,req->count);
					req->data = NULL;
				}
				else {
					/* map the buffer we have to copy it to */
					req->data = cache_alloc(req->count);
					if(req->data)
						memcpy(req->data,data,req->count);
				}
			}
			req->state = REQ_STATE_FINISHED;
			vfs_req_remove(req);
			/* the thread can continue now */
			ev_wakeupThread(req->tid,EV_REQ_REPLY);
		}
	}
}

static void vfs_real_statRespHandler(sVFSNode *node,const void *data,size_t size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->data))
		return;

	/* find the request for the node */
	req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->data.arg1;
		req->data = cache_alloc(sizeof(sFileInfo));
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
		vfs_req_remove(req);
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static void vfs_real_defRespHandler(sVFSNode *node,const void *data,size_t size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the node */
	req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		vfs_req_remove(req);
		/* the thread can continue now */
		ev_wakeupThread(req->tid,EV_REQ_REPLY);
	}
}

static file_t vfs_real_requestFile(pid_t pid,sVFSNode **node) {
	int err;
	sSLNode *n;
	sFSChan *chan;
	sVFSNode *child;
	inode_t nodeNo;
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

	chan = (sFSChan*)cache_alloc(sizeof(sFSChan));
	if(!chan)
		return ERR_NOT_ENOUGH_MEM;
	chan->active = true;

	/* resolve path to fs */
	err = vfs_node_resolvePath(FS_PATH,&nodeNo,NULL,VFS_READ | VFS_WRITE | VFS_MSGS);
	if(err < 0)
		goto errorChan;

	/* create usage-node */
	chan->node = vfs_node_get(nodeNo);
	child = vfs_chan_create(pid,chan->node);
	if(child == NULL) {
		err = ERR_NOT_ENOUGH_MEM;
		goto errorChan;
	}
	chan->node = child;
	nodeNo = vfs_node_getNo(child);

	/* open file */
	err = vfs_openFile(pid,VFS_READ | VFS_WRITE | VFS_MSGS,nodeNo,VFS_DEV_NO);
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
	cache_free(chan);
	return err;
}

static void vfs_real_releaseFile(pid_t pid,file_t file) {
	sSLNode *n;
	const sProc *p = proc_getByPid(pid);
	for(n = sll_begin(p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		if(chan->file == file) {
			chan->active = false;
			return;
		}
	}
}
