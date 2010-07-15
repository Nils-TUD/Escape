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
#include <sys/task/sched.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/request.h>
#include <sys/vfs/real.h>
#include <sys/kevent.h>
#include <sys/util.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errors.h>

#define FS_PATH				"/dev/fs"
#define R2V_MAP_SIZE		64

typedef struct {
	tFileNo virtFile;
	tFileNo realFile;
} sReal2Virt;

static s32 vfsr_doStat(tTid tid,const char *path,tInodeNo ino,tDevNo devNo,sFileInfo *info);
/* The request-handler for sending a path and receiving a result */
static s32 vfsr_pathReqHandler(tTid tid,const char *path1,const char *path2,u32 arg1,u32 cmd);
/* The response-handler for the different message-ids */
static void vfsr_openRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static void vfsr_readRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static void vfsr_statRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static void vfsr_defRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size);
static tFileNo vfsr_create(tTid tid);
static s32 vfsr_add(tFileNo virtFile,tFileNo realFile);
static sReal2Virt *vfsr_get(tTid tid,tFileNo real,s32 *err);
static void vfsr_remove(tTid tid,tFileNo real);
static void vfsr_destroy(tTid tid,tFileNo virt);

static sMsg msg;
static sSLList *real2virt[R2V_MAP_SIZE] = {NULL};

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

s32 vfsr_openFile(tTid tid,u16 flags,const char *path) {
	s32 res;
	u32 pathLen = strlen(path);
	tFileNo virtFile,realFile;
	sRequest *req;

	if(pathLen > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
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
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL) {
		vfsr_destroy(tid,virtFile);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* process is now running again, and we've received the reply */
	/* that's magic, isn't it? ;D */

	/* error? */
	if((s32)req->count < 0) {
		tInodeNo no = req->count;
		vfsreq_remRequest(req);
		vfsr_destroy(tid,virtFile);
		return no;
	}

	/* now open the file */
	realFile = vfs_openFile(tid,flags,(tInodeNo)req->count,(tDevNo)req->val1);
	vfsreq_remRequest(req);
	if((res = vfsr_add(virtFile,realFile)) < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}
	return realFile;
}

s32 vfsr_openInode(tTid tid,u16 flags,tInodeNo ino,tDevNo dev) {
	s32 res;
	tFileNo virtFile,realFile;

	virtFile = vfsr_create(tid);
	if(virtFile < 0)
		return virtFile;

	/* TODO maybe we should send an open-msg to fs, too but in a different form? */

	/* now open the file */
	realFile = vfs_openFile(tid,flags,ino,dev);
	if((res = vfsr_add(virtFile,realFile)) < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}
	return realFile;
}

s32 vfsr_istat(tTid tid,tInodeNo ino,tDevNo devNo,sFileInfo *info) {
	return vfsr_doStat(tid,NULL,ino,devNo,info);
}

s32 vfsr_stat(tTid tid,const char *path,sFileInfo *info) {
	return vfsr_doStat(tid,path,0,0,info);
}

static s32 vfsr_doStat(tTid tid,const char *path,tInodeNo ino,tDevNo devNo,sFileInfo *info) {
	s32 res;
	u32 pathLen = 0;
	tFileNo virtFile;
	sRequest *req;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return ERR_INVALID_ARGS;
	}
	virtFile = vfsr_create(tid);
	if(virtFile < 0)
		return virtFile;

	/* send msg to fs */
	if(path) {
		memcpy(msg.str.s1,path,pathLen + 1);
		res = vfs_sendMsg(tid,virtFile,MSG_FS_STAT,(u8*)&msg,sizeof(msg.str));
	}
	else {
		msg.args.arg1 = ino;
		msg.args.arg2 = devNo;
		res = vfs_sendMsg(tid,virtFile,MSG_FS_ISTAT,(u8*)&msg,sizeof(msg.args));
	}
	if(res < 0) {
		vfsr_destroy(tid,virtFile);
		return res;
	}

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL) {
		vfsr_destroy(tid,virtFile);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* error? */
	vfsr_destroy(tid,virtFile);
	if((s32)req->count < 0) {
		s32 no = req->count;
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

s32 vfsr_readFile(tTid tid,tFileNo file,tInodeNo inodeNo,tDevNo devNo,u8 *buffer,u32 offset,
		u32 count) {
	sRequest *req;
	sReal2Virt *r2v;
	u32 pcount,*frameNos;
	s32 res;

	r2v = vfsr_get(tid,file,&res);
	if(r2v == NULL)
		return res;

	/* send msg to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(tid,r2v->virtFile,MSG_FS_READ,(u8*)&msg,sizeof(msg.args));
	if(res < 0)
		return res;

#if 0
	/* get the frame-numbers which we'll map later to write the reply directly to the calling
	 * process */
	pcount = BYTES_2_PAGES(((u32)buffer & (PAGE_SIZE - 1)) + count);
	frameNos = kheap_alloc(pcount * sizeof(u32));
	if(frameNos == NULL)
		return ERR_NOT_ENOUGH_MEM;
	paging_getFrameNos(frameNos,(u32)buffer,count);

	/* wait for a reply */
	req = vfsreq_waitForReadReply(tid,count,frameNos,pcount,(u32)buffer % PAGE_SIZE);
#endif
	req = vfsreq_waitForReply(tid,buffer,count);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	if(req->readFrNos) {
		memcpy(buffer,req->readFrNos,req->count);
		kheap_free(req->readFrNos);
	}
	vfsreq_remRequest(req);
	return res;
}

s32 vfsr_writeFile(tTid tid,tFileNo file,tInodeNo inodeNo,tDevNo devNo,const u8 *buffer,u32 offset,
		u32 count) {
	sRequest *req;
	sReal2Virt *r2v;
	s32 res;

	r2v = vfsr_get(tid,file,&res);
	if(r2v == NULL)
		return res;

	/* send msg first */
	msg.data.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	msg.args.arg3 = offset;
	msg.args.arg4 = count;
	res = vfs_sendMsg(tid,r2v->virtFile,MSG_FS_WRITE,(u8*)&msg,sizeof(msg.data));
	if(res < 0)
		return res;
	/* now send data */
	res = vfs_sendMsg(tid,r2v->virtFile,MSG_FS_WRITE,buffer,count);
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

s32 vfsr_link(tTid tid,const char *oldPath,const char *newPath) {
	return vfsr_pathReqHandler(tid,oldPath,newPath,0,MSG_FS_LINK);
}

s32 vfsr_unlink(tTid tid,const char *path) {
	return vfsr_pathReqHandler(tid,path,NULL,0,MSG_FS_UNLINK);
}

s32 vfsr_mkdir(tTid tid,const char *path) {
	return vfsr_pathReqHandler(tid,path,NULL,0,MSG_FS_MKDIR);
}

s32 vfsr_rmdir(tTid tid,const char *path) {
	return vfsr_pathReqHandler(tid,path,NULL,0,MSG_FS_RMDIR);
}

s32 vfsr_mount(tTid tid,const char *device,const char *path,u16 type) {
	return vfsr_pathReqHandler(tid,device,path,type,MSG_FS_MOUNT);
}

s32 vfsr_unmount(tTid tid,const char *path) {
	return vfsr_pathReqHandler(tid,path,NULL,0,MSG_FS_UNMOUNT);
}

s32 vfsr_sync(tTid tid) {
	s32 res;
	tFileNo virtFile = vfsr_create(tid);
	if(virtFile < 0)
		return virtFile;

	res = vfs_sendMsg(tid,virtFile,MSG_FS_SYNC,(u8*)&msg,sizeof(msg.args));
	vfsr_destroy(tid,virtFile);
	return res;
}

void vfsr_closeFile(tTid tid,tFileNo file,tInodeNo inodeNo,tDevNo devNo) {
	s32 res;
	sReal2Virt *r2v;
	r2v = vfsr_get(tid,file,&res);
	if(r2v == NULL)
		return;

	/* write message to fs */
	msg.args.arg1 = inodeNo;
	msg.args.arg2 = devNo;
	vfs_sendMsg(tid,r2v->virtFile,MSG_FS_CLOSE,(u8*)&msg,sizeof(msg.args));
	/* no response necessary, therefore no wait, too */

	/* remove the stuff for the communication */
	vfsr_remove(tid,file);
}

static s32 vfsr_pathReqHandler(tTid tid,const char *path1,const char *path2,u32 arg1,u32 cmd) {
	s32 res;
	sRequest *req;
	tFileNo virtFile;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return ERR_INVALID_ARGS;

	if((virtFile = vfsr_create(tid)) < 0)
		return virtFile;

	strcpy(msg.str.s1,path1);
	if(path2)
		strcpy(msg.str.s2,path2);
	msg.str.arg1 = arg1;
	res = vfs_sendMsg(tid,virtFile,cmd,(u8*)&msg,sizeof(msg.str));
	if(res < 0)
		return res;

	/* wait for a reply */
	req = vfsreq_waitForReply(tid,NULL,0);
	vfsr_destroy(tid,virtFile);
	if(req == NULL)
		return ERR_NOT_ENOUGH_MEM;

	res = req->count;
	vfsreq_remRequest(req);
	return res;
}

static void vfsr_openRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		req->val1 = rmsg->args.arg2;
		/* the thread can continue now */
		thread_wakeup(tid,EV_REQ_REPLY);
	}
}

static void vfsr_readRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	/* find the request for the tid */
	sRequest *req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* the first one is the message */
		if(req->state == REQ_STATE_WAITING) {
			sMsg *rmsg = (sMsg*)data;
			/* an error? */
			if(!data || size < sizeof(rmsg->args) || (s32)rmsg->args.arg1 <= 0) {
				req->count = 0;
				req->state = REQ_STATE_FINISHED;
				thread_wakeup(tid,EV_REQ_REPLY);
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
				req->readFrNos = (u32*)kheap_alloc(req->count);
				if(req->readFrNos)
					memcpy(req->readFrNos,data,req->count);
			}
#if 0
			u8 *addr = (u8*)TEMP_MAP_AREA;
			paging_map(TEMP_MAP_AREA,req->readFrNos,req->readFrNoCount,
					PG_PRESENT | PG_SUPERVISOR | PG_WRITABLE);
			memcpy(addr + req->readOffset,data,req->count);
			/* unmap it and free the frame-nos */
			paging_unmap(TEMP_MAP_AREA,req->readFrNoCount,false);
			kheap_free(req->readFrNos);
#endif
			req->state = REQ_STATE_FINISHED;
			/* the thread can continue now */
			thread_wakeup(tid,EV_REQ_REPLY);
		}
	}
}

static void vfsr_statRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->data))
		return;

	/* find the request for the tid */
	req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the inode-number */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->data.arg1;
		req->data = (void*)kheap_alloc(sizeof(sFileInfo));
		if(req->data != NULL)
			memcpy(req->data,rmsg->data.d,sizeof(sFileInfo));
		/* the thread can continue now */
		thread_wakeup(tid,EV_REQ_REPLY);
	}
}

static void vfsr_defRespHandler(tTid tid,sVFSNode *node,const u8 *data,u32 size) {
	UNUSED(node);
	sMsg *rmsg = (sMsg*)data;
	sRequest *req;
	if(!data || size < sizeof(rmsg->args))
		return;

	/* find the request for the tid */
	req = vfsreq_getRequestByPid(tid);
	if(req != NULL) {
		/* remove request and give him the result */
		req->state = REQ_STATE_FINISHED;
		req->count = rmsg->args.arg1;
		/* the thread can continue now */
		thread_wakeup(tid,EV_REQ_REPLY);
	}
}

static tFileNo vfsr_create(tTid tid) {
	s32 res;
	tInodeNo nodeNo;

	/* create a virtual node for communication with fs */
	if((res = vfsn_resolvePath(FS_PATH,&nodeNo,NULL,VFS_CONNECT)) != 0)
		return res;
	/* open the file */
	return vfs_openFile(tid,VFS_READ | VFS_WRITE,nodeNo,VFS_DEV_NO);
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

static sReal2Virt *vfsr_get(tTid tid,tFileNo real,s32 *err) {
	sSLNode *n;
	sReal2Virt *r2v;
	s32 owner;
	sSLList *list = real2virt[real % R2V_MAP_SIZE];
	bool foundReal = false;
	*err = ERR_INVALID_INODENO;
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		r2v = (sReal2Virt*)n->data;
		if(r2v->realFile == real) {
			owner = vfs_getOwner(r2v->virtFile);
			/* if we're not the owner, we can't use the file but remember that the real-file exists */
			if(owner != tid) {
				foundReal = true;
				continue;
			}
			*err = 0;
			return (sReal2Virt*)n->data;
		}
	}
	/* if we're here and have found the real-file, we were not the owner.
	 * that means that this file has been inherited, so we have to create a new virt-file and
	 * use that (for accessing the fs-driver). */
	if(foundReal) {
		tFileNo virt = vfsr_create(tid);
		if(virt >= 0) {
			if(vfsr_add(virt,real) == 0)
				return vfsr_get(tid,real,err);
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
