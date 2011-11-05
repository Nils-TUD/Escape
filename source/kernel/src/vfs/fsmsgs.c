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
#include <sys/vfs/fsmsgs.h>
#include <sys/vfs/channel.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errno.h>

#define FS_PATH				"/dev/fs"

typedef struct {
	uint8_t active;
	file_t file;
	sVFSNode *node;
} sFSChan;

/* for istat() and stat() */
static int vfs_fsmsgs_doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,sFileInfo *info);
/* The request-handler for sending a path and receiving a result */
static int vfs_fsmsgs_pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd);

/* for requesting and releasing a file for communication */
static file_t vfs_fsmsgs_requestFile(pid_t pid,sVFSNode **node);
static void vfs_fsmsgs_releaseFile(pid_t pid,file_t file);

static klock_t fsChanLock;

void vfs_fsmsgs_removeProc(pid_t pid) {
	sProc *p = proc_getByPid(pid);
	sSLNode *n;
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		vfs_closeFile(pid,chan->file);
	}
	sll_clear(&p->fsChans,true);
}

file_t vfs_fsmsgs_openPath(pid_t pid,uint flags,const char *path) {
	const sProc *p = proc_getByPid(pid);
	ssize_t res = -ENOMEM;
	size_t pathLen = strlen(path);
	sVFSNode *node;
	file_t fs;
	sStrMsg msg;
	inode_t inode;
	dev_t dev;

	if(pathLen > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;
	fs = vfs_fsmsgs_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* send msg to fs */
	msg.arg1 = flags;
	msg.arg2 = p->euid;
	msg.arg3 = p->egid;
	msg.arg4 = p->pid;
	memcpy(msg.s1,path,pathLen + 1);
	res = vfs_sendMsg(pid,fs,MSG_FS_OPEN,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* receive response */
	do
		res = vfs_receiveMsg(pid,fs,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;

	inode = msg.arg1;
	dev = msg.arg2;
	if(inode < 0) {
		res = inode;
		goto error;
	}

	/* now open the file */
	res = vfs_openFile(pid,flags,inode,dev);

error:
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

int vfs_fsmsgs_istat(pid_t pid,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	return vfs_fsmsgs_doStat(pid,NULL,ino,devNo,info);
}

int vfs_fsmsgs_stat(pid_t pid,const char *path,USER sFileInfo *info) {
	return vfs_fsmsgs_doStat(pid,path,0,0,info);
}

static int vfs_fsmsgs_doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	ssize_t res = -ENOMEM;
	size_t pathLen = 0;
	file_t fs;
	sVFSNode *node;
	sMsg msg;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return -ENAMETOOLONG;
	}
	fs = vfs_fsmsgs_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* send msg to fs */
	if(path) {
		const sProc *p = proc_getByPid(pid);
		msg.str.arg1 = p->euid;
		msg.str.arg2 = p->egid;
		msg.str.arg3 = p->pid;
		memcpy(msg.str.s1,path,pathLen + 1);
		res = vfs_sendMsg(pid,fs,MSG_FS_STAT,&msg,sizeof(msg.str),NULL,0);
	}
	else {
		msg.args.arg1 = ino;
		msg.args.arg2 = devNo;
		res = vfs_sendMsg(pid,fs,MSG_FS_ISTAT,&msg,sizeof(msg.args),NULL,0);
	}
	if(res < 0)
		goto error;

	/* receive response */
	do
		res = vfs_receiveMsg(pid,fs,NULL,&msg,sizeof(msg.data),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.data.arg1;

	/* copy file-info */
	if(res == 0)
		memcpy(info,msg.data.d,sizeof(sFileInfo));

error:
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

ssize_t vfs_fsmsgs_read(pid_t pid,inode_t inodeNo,dev_t devNo,USER void *buffer,off_t offset,
		size_t count) {
	ssize_t res;
	sVFSNode *node;
	sArgsMsg msg;
	file_t fs = vfs_fsmsgs_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* send msg to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = vfs_receiveMsg(pid,fs,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;

	/* handle response */
	res = msg.arg1;
	if(res <= 0)
		goto error;

	/* read data */
	do
		res = vfs_receiveMsg(pid,fs,NULL,buffer,count,true);
	while(res == -EINTR);

error:
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

ssize_t vfs_fsmsgs_write(pid_t pid,inode_t inodeNo,dev_t devNo,USER const void *buffer,off_t offset,
		size_t count) {
	ssize_t res = -ENOMEM;
	sVFSNode *node;
	sArgsMsg msg;
	file_t fs = vfs_fsmsgs_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* send msg and data */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = vfs_sendMsg(pid,fs,MSG_FS_WRITE,&msg,sizeof(msg),buffer,count);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = vfs_receiveMsg(pid,fs,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.arg1;

error:
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

int vfs_fsmsgs_chmod(pid_t pid,const char *path,mode_t mode) {
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,mode,MSG_FS_CHMOD);
}

int vfs_fsmsgs_chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	/* TODO better solution? */
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,(uid << 16) | (gid & 0xFFFF),MSG_FS_CHOWN);
}

int vfs_fsmsgs_link(pid_t pid,const char *oldPath,const char *newPath) {
	return vfs_fsmsgs_pathReqHandler(pid,oldPath,newPath,0,MSG_FS_LINK);
}

int vfs_fsmsgs_unlink(pid_t pid,const char *path) {
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,0,MSG_FS_UNLINK);
}

int vfs_fsmsgs_mkdir(pid_t pid,const char *path) {
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,0,MSG_FS_MKDIR);
}

int vfs_fsmsgs_rmdir(pid_t pid,const char *path) {
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,0,MSG_FS_RMDIR);
}

int vfs_fsmsgs_mount(pid_t pid,const char *device,const char *path,uint type) {
	return vfs_fsmsgs_pathReqHandler(pid,device,path,type,MSG_FS_MOUNT);
}

int vfs_fsmsgs_unmount(pid_t pid,const char *path) {
	return vfs_fsmsgs_pathReqHandler(pid,path,NULL,0,MSG_FS_UNMOUNT);
}

int vfs_fsmsgs_sync(pid_t pid) {
	int res;
	file_t fs = vfs_fsmsgs_requestFile(pid,NULL);
	if(fs < 0)
		return fs;
	res = vfs_sendMsg(pid,fs,MSG_FS_SYNC,NULL,0,NULL,0);
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

void vfs_fsmsgs_close(pid_t pid,inode_t inodeNo,dev_t devNo) {
	sArgsMsg msg;
	file_t fs = vfs_fsmsgs_requestFile(pid,NULL);
	if(fs < 0)
		return;

	/* write message to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	vfs_sendMsg(pid,fs,MSG_FS_CLOSE,&msg,sizeof(msg),NULL,0);
	/* no response necessary, therefore no wait, too */
	vfs_fsmsgs_releaseFile(pid,fs);
}

static int vfs_fsmsgs_pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd) {
	int res = -ENOMEM;
	const sProc *p = proc_getByPid(pid);
	file_t fs;
	sVFSNode *node;
	sStrMsg msg;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;

	fs = vfs_fsmsgs_requestFile(pid,&node);
	if(fs < 0)
		return fs;

	/* send msg */
	strcpy(msg.s1,path1);
	if(path2)
		strcpy(msg.s2,path2);
	msg.arg1 = arg1;
	msg.arg2 = p->euid;
	msg.arg3 = p->egid;
	msg.arg4 = p->pid;
	res = vfs_sendMsg(pid,fs,cmd,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = vfs_receiveMsg(pid,fs,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.arg1;

error:
	vfs_fsmsgs_releaseFile(pid,fs);
	return res;
}

static file_t vfs_fsmsgs_requestFile(pid_t pid,sVFSNode **node) {
	int err;
	sSLNode *n;
	sFSChan *chan;
	sVFSNode *child,*fsnode;
	inode_t nodeNo;
	sProc *p = proc_getByPid(pid);
	/* check if there's a free channel */
	spinlock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		chan = (sFSChan*)n->data;
		if(!chan->active) {
			if(chan->node->name == NULL) {
				/* remove channel */
				sll_removeFirstWith(&p->fsChans,chan);
				cache_free(chan);
				spinlock_release(&fsChanLock);
				return -EDESTROYED;
			}
			if(node)
				*node = chan->node;
			chan->active = true;
			spinlock_release(&fsChanLock);
			return chan->file;
		}
	}
	spinlock_release(&fsChanLock);

	chan = (sFSChan*)cache_alloc(sizeof(sFSChan));
	if(!chan)
		return -ENOMEM;
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
		err = -ENOMEM;
		goto errorNode;
	}
	chan->node = child;
	nodeNo = vfs_node_getNo(child);

	/* open file */
	err = vfs_openFile(pid,VFS_READ | VFS_WRITE | VFS_MSGS,nodeNo,VFS_DEV_NO);
	if(err < 0)
		goto errorChild;
	chan->file = err;
	spinlock_aquire(&fsChanLock);
	if(!sll_append(&p->fsChans,chan)) {
		spinlock_release(&fsChanLock);
		goto errorClose;
	}
	spinlock_release(&fsChanLock);
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

static void vfs_fsmsgs_releaseFile(pid_t pid,file_t file) {
	sSLNode *n;
	const sProc *p = proc_getByPid(pid);
	spinlock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		if(chan->file == file) {
			chan->active = false;
			break;
		}
	}
	spinlock_release(&fsChanLock);
}

void vfs_fsmsgs_printFSChans(const sProc *p) {
	sSLNode *n;
	spinlock_aquire(&fsChanLock);
	for(n = sll_begin(&p->fsChans); n != NULL; n = n->next) {
		sFSChan *chan = (sFSChan*)n->data;
		vid_printf("\t\t%s (%s)\n",vfs_node_getPath(vfs_node_getNo(chan->node)),
				chan->active ? "active" : "not active");
	}
	spinlock_release(&fsChanLock);
}
