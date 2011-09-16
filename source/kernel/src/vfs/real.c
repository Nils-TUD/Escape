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
#include <sys/mem/vmm.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/request.h>
#include <sys/vfs/real.h>
#include <sys/vfs/channel.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/klock.h>
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

static klock_t fsChanLock;

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
	sSLNode *n;
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		vfs_closeFile(pid,chan->file);
	}
	sll_clear(&p->fsChans,true);
}

file_t vfs_real_openPath(pid_t pid,uint flags,const char *path) {
	const sProc *p = proc_getByPid(pid);
	ssize_t res = ERR_NOT_ENOUGH_MEM;
	size_t pathLen = strlen(path);
	sVFSNode *node;
	file_t fs;
	sRequest *req;
	sStrMsg msg;

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
	msg.arg1 = flags;
	msg.arg2 = p->euid;
	msg.arg3 = p->egid;
	msg.arg4 = p->pid;
	memcpy(msg.s1,path,pathLen + 1);
	res = vfs_sendMsg(pid,fs,MSG_FS_OPEN,&msg,sizeof(msg));
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

int vfs_real_istat(pid_t pid,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	return vfs_real_doStat(pid,NULL,ino,devNo,info);
}

int vfs_real_stat(pid_t pid,const char *path,USER sFileInfo *info) {
	return vfs_real_doStat(pid,path,0,0,info);
}

static int vfs_real_doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	int res = ERR_NOT_ENOUGH_MEM;
	void *data;
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
		sStrMsg msg;
		const sProc *p = proc_getByPid(pid);
		msg.arg1 = p->euid;
		msg.arg2 = p->egid;
		msg.arg3 = p->pid;
		memcpy(msg.s1,path,pathLen + 1);
		res = vfs_sendMsg(pid,fs,MSG_FS_STAT,&msg,sizeof(msg));
	}
	else {
		sArgsMsg msg;
		msg.arg1 = ino;
		msg.arg2 = devNo;
		res = vfs_sendMsg(pid,fs,MSG_FS_ISTAT,&msg,sizeof(msg));
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

	/* release resources before memcpy */
	data = req->data;
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);

	/* copy to info-struct */
	if(data) {
		sProc *p = proc_request(proc_getRunning(),PLOCK_REGIONS);
		if(!vmm_makeCopySafe(p,info,sizeof(sFileInfo))) {
			proc_release(p,PLOCK_REGIONS);
			cache_free(data);
			return ERR_INVALID_ARGS;
		}
		memcpy((void*)info,data,sizeof(sFileInfo));
		proc_release(p,PLOCK_REGIONS);
		cache_free(data);
	}
	return 0;

error:
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	return res;
}

ssize_t vfs_real_read(pid_t pid,inode_t inodeNo,dev_t devNo,USER void *buffer,off_t offset,
		size_t count) {
	sRequest *req;
	ssize_t res;
	void *data;
	sVFSNode *node;
	sArgsMsg msg;
	file_t fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request */
	req = vfs_req_get(node,buffer,0);
	if(!req) {
		vfs_real_releaseFile(pid,fs);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* send msg to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_READ,&msg,sizeof(msg));
	if(res < 0) {
		vfs_req_free(req);
		vfs_real_releaseFile(pid,fs);
		return res;
	}

	/* wait for a reply */
	vfs_real_wait(req);

	/* release resources before memcpy */
	res = req->count;
	data = req->data;
	vfs_req_free(req);
	vfs_real_releaseFile(pid,fs);
	if(data) {
		thread_addHeapAlloc(data);
		memcpy(buffer,data,res);
		thread_remHeapAlloc(data);
		cache_free(data);
	}
	return res;
}

ssize_t vfs_real_write(pid_t pid,inode_t inodeNo,dev_t devNo,USER const void *buffer,off_t offset,
		size_t count) {
	sRequest *req;
	ssize_t res = ERR_NOT_ENOUGH_MEM;
	sVFSNode *node;
	sArgsMsg msg;
	file_t fs = vfs_real_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* get request; maybe we have to wait */
	req = vfs_req_get(node,NULL,0);
	if(!req)
		goto error;

	/* send msg first */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,&msg,sizeof(msg));
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
	res = vfs_sendMsg(pid,fs,MSG_FS_SYNC,NULL,0);
	vfs_real_releaseFile(pid,fs);
	return res;
}

void vfs_real_close(pid_t pid,inode_t inodeNo,dev_t devNo) {
	sArgsMsg msg;
	file_t fs = vfs_real_requestFile(pid,NULL);
	if(fs < 0)
		return;

	/* write message to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	vfs_sendMsg(pid,fs,MSG_FS_CLOSE,&msg,sizeof(msg));
	/* no response necessary, therefore no wait, too */
	vfs_real_releaseFile(pid,fs);
}

static int vfs_real_pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd) {
	int res = ERR_NOT_ENOUGH_MEM;
	const sProc *p = proc_getByPid(pid);
	sRequest *req;
	file_t fs;
	sVFSNode *node;
	sStrMsg msg;

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
	strcpy(msg.s1,path1);
	if(path2)
		strcpy(msg.s2,path2);
	msg.arg1 = arg1;
	msg.arg2 = p->euid;
	msg.arg3 = p->egid;
	msg.arg4 = p->pid;
	res = vfs_sendMsg(pid,fs,cmd,&msg,sizeof(msg));
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

static void vfs_real_openRespHandler(sVFSNode *node,USER const void *data,A_UNUSED size_t size) {
	sMsg *rmsg = (sMsg*)data;
	inode_t inode = rmsg->args.arg1;
	dev_t dev = rmsg->args.arg2;
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = inode;
		req->val1 = dev;
		/* the thread can continue now */
		ev_wakeupThread(req->thread,EV_REQ_REPLY);
		vfs_req_remove(req);
	}
}

static void vfs_real_readRespHandler(sVFSNode *node,USER const void *data,A_UNUSED size_t size) {
	/* find the request for the node */
	sRequest *req = vfs_req_getByNode(node);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			ulong res = rmsg->args.arg1;
			/* an error? */
			if((long)res <= 0) {
				req->count = 0;
				req->state = REQ_STATE_FINISHED;
				req->data = NULL;
				ev_wakeupThread(req->thread,EV_REQ_REPLY);
				vfs_req_remove(req);
				return;
			}
			/* otherwise we'll receive the data with the next msg */
			req->count = res;
			req->state = REQ_STATE_WAIT_DATA;
			vfs_req_release(req);
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
					if(req->data) {
						thread_addHeapAlloc(req->data);
						memcpy(req->data,data,req->count);
						thread_remHeapAlloc(req->data);
					}
				}
			}
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			ev_wakeupThread(req->thread,EV_REQ_REPLY);
			vfs_req_remove(req);
		}
		else
			vfs_req_release(req);
	}
}

static void vfs_real_statRespHandler(sVFSNode *node,USER const void *data,A_UNUSED size_t size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req = vfs_req_getByNode(node);
	ulong res = rmsg->data.arg1;
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = res;
		if(res == 0) {
			req->data = cache_alloc(sizeof(sFileInfo));
			if(req->data != NULL) {
				thread_addHeapAlloc(req->data);
				memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
				thread_remHeapAlloc(req->data);
			}
		}
		/* the thread can continue now */
		ev_wakeupThread(req->thread,EV_REQ_REPLY);
		vfs_req_remove(req);
	}
}

static void vfs_real_defRespHandler(sVFSNode *node,USER const void *data,A_UNUSED size_t size) {
	sMsg *rmsg = (sMsg*)data;
	sRequest *req = vfs_req_getByNode(node);
	ulong res = rmsg->data.arg1;
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = res;
		/* the thread can continue now */
		ev_wakeupThread(req->thread,EV_REQ_REPLY);
		vfs_req_remove(req);
	}
}

static file_t vfs_real_requestFile(pid_t pid,sVFSNode **node) {
	int err;
	sSLNode *n;
	sFSChan *chan;
	sVFSNode *child,*fsnode;
	inode_t nodeNo;
	sProc *p = proc_getByPid(pid);
	/* check if there's a free channel */
	klock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		chan = (sFSChan*)n->data;
		if(!chan->active) {
			if(chan->node->name == NULL) {
				/* remove channel */
				sll_removeFirstWith(&p->fsChans,chan);
				cache_free(chan);
				klock_release(&fsChanLock);
				return ERR_NODE_DESTROYED;
			}
			if(node)
				*node = chan->node;
			chan->active = true;
			klock_release(&fsChanLock);
			return chan->file;
		}
	}
	klock_release(&fsChanLock);

	chan = (sFSChan*)cache_alloc(sizeof(sFSChan));
	if(!chan)
		return ERR_NOT_ENOUGH_MEM;
	chan->active = true;

	/* resolve path to fs */
	err = vfs_node_resolvePath(FS_PATH,&nodeNo,NULL,VFS_READ | VFS_WRITE | VFS_MSGS);
	if(err < 0)
		goto errorChan;

	/* create usage-node */
	fsnode = vfs_node_request(nodeNo);
	if(!fsnode)
		goto errorChan;
	child = vfs_chan_create(pid,fsnode);
	if(child == NULL) {
		err = ERR_NOT_ENOUGH_MEM;
		goto errorNode;
	}
	chan->node = child;
	nodeNo = vfs_node_getNo(child);

	/* open file */
	err = vfs_openFile(pid,VFS_READ | VFS_WRITE | VFS_MSGS,nodeNo,VFS_DEV_NO);
	if(err < 0)
		goto errorChild;
	chan->file = err;
	klock_aquire(&fsChanLock);
	if(!sll_append(&p->fsChans,chan)) {
		klock_release(&fsChanLock);
		goto errorClose;
	}
	klock_release(&fsChanLock);
	if(node)
		*node = chan->node;
	vfs_node_release(fsnode);
	return chan->file;

errorClose:
	vfs_closeFile(pid,chan->file);
errorChild:
	vfs_node_destroy(child);
errorNode:
	vfs_node_release(fsnode);
errorChan:
	cache_free(chan);
	return err;
}

static void vfs_real_releaseFile(pid_t pid,file_t file) {
	sSLNode *n;
	const sProc *p = proc_getByPid(pid);
	klock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		if(chan->file == file) {
			chan->active = false;
			break;
		}
	}
	klock_release(&fsChanLock);
}

void vfs_real_printFSChans(const sProc *p) {
	sSLNode *n;
	klock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		vid_printf("\t\t%s (%s)\n",vfs_node_getPath(vfs_node_getNo(chan->node)),
				chan->active ? "active" : "not active");
	}
	klock_release(&fsChanLock);
}
