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
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/fsmsgs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/request.h>
#include <sys/vfs/devmsgs.h>
#include <sys/vfs/file.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/pipe.h>
#include <sys/vfs/device.h>
#include <sys/task/proc.h>
#include <sys/task/groups.h>
#include <sys/task/event.h>
#include <sys/task/lock.h>
#include <sys/task/timer.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define FILE_COUNT					((file_t)(gftArray.objCount))

/* an entry in the global file table */
typedef struct sGFTEntry {
	klock_t lock;
	/* read OR write; flags = 0 => entry unused */
	ushort flags;
	/* the owner of this file */
	pid_t owner;
	/* number of references (file-descriptors) */
	ushort refCount;
	/* number of threads that are currently using this file (reading, writing, ...) */
	ushort usageCount;
	/* current position in file */
	off_t position;
	/* node-number */
	inode_t nodeNo;
	/* the node, if devNo == VFS_DEV_NO, otherwise null */
	sVFSNode *node;
	/* the device-number */
	dev_t devNo;
	/* for the freelist */
	struct sGFTEntry *next;
} sGFTEntry;

static bool vfs_doCloseFile(pid_t pid,file_t file,sGFTEntry *e);
static file_t vfs_getFreeFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,sVFSNode *n);
static void vfs_releaseFile(sGFTEntry *e);

/* global file table (expands dynamically) */
static sDynArray gftArray;
static sGFTEntry *gftFreeList;
static sVFSNode *procsNode;
static sVFSNode *devNode;
static klock_t gftLock;
static klock_t waitLock;

void vfs_init(void) {
	sVFSNode *root,*sys;
	dyna_start(&gftArray,sizeof(sGFTEntry),GFT_AREA,GFT_AREA_SIZE);
	gftFreeList = NULL;
	vfs_node_init();

	/*
	 *  /
	 *   |- system
	 *   |   |-pipe
	 *   |   |-processes
	 *   |   \-devices
	 *   \- dev
	 */
	root = vfs_dir_create(KERNEL_PID,NULL,(char*)"");
	sys = vfs_dir_create(KERNEL_PID,root,(char*)"system");
	vfs_dir_create(KERNEL_PID,sys,(char*)"pipe");
	procsNode = vfs_dir_create(KERNEL_PID,sys,(char*)"processes");
	vfs_dir_create(KERNEL_PID,sys,(char*)"devices");
	devNode = vfs_dir_create(KERNEL_PID,root,(char*)"dev");

	vfs_info_init();
	vfs_req_init();
	vfs_devmsgs_init();
	vfs_fsmsgs_init();
}

int vfs_hasAccess(pid_t pid,sVFSNode *n,ushort flags) {
	const sProc *p;
	uint mode;
	if(n->name == NULL)
		return -ENOENT;
	/* kernel is allmighty :P */
	if(pid == KERNEL_PID)
		return 0;

	p = proc_getByPid(pid);
	if(p == NULL)
		return -ESRCH;
	/* root is (nearly) allmighty as well */
	if(p->euid == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(flags & VFS_EXEC)
			return (n->mode & MODE_EXEC) ? 0 : -EACCES;
		return 0;
	}

	/* determine mask */
	if(p->euid == n->uid)
		mode = n->mode & S_IRWXU;
	else if(p->egid == n->gid || groups_contains(p->pid,n->gid))
		mode = n->mode & S_IRWXG;
	else
		mode = n->mode & S_IRWXO;

	/* check access */
	if((flags & VFS_READ) && !(mode & MODE_READ))
		return -EACCES;
	if((flags & VFS_WRITE) && !(mode & MODE_WRITE))
		return -EACCES;
	if((flags & VFS_EXEC) && !(mode & MODE_EXEC))
		return -EACCES;
	return 0;
}

static sGFTEntry *vfs_getGFTEntry(file_t file) {
	return (sGFTEntry*)dyna_getObj(&gftArray,file);
}

bool vfs_isDevice(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	return e->flags & VFS_DEVICE;
}

void vfs_incRefs(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	klock_aquire(&e->lock);
	e->refCount++;
	klock_release(&e->lock);
}

void vfs_incUsages(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	klock_aquire(&e->lock);
	e->usageCount++;
	klock_release(&e->lock);
}

void vfs_decUsages(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	klock_aquire(&e->lock);
	assert(e->usageCount > 0);
	e->usageCount--;
	/* if it should be closed in the meanwhile, we have to close it now, because it wasn't possible
	 * previously because of our usage */
	if(e->usageCount == 0 && e->refCount == 0)
		vfs_doCloseFile(proc_getRunning(),file,e);
	klock_release(&e->lock);
}

int vfs_fcntl(A_UNUSED pid_t pid,file_t file,uint cmd,int arg) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	switch(cmd) {
		case F_GETACCESS:
			return e->flags & (VFS_READ | VFS_WRITE | VFS_MSGS);
		case F_GETFL:
			return e->flags & VFS_NOBLOCK;
		case F_SETFL:
			klock_aquire(&e->lock);
			e->flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_CREATE | VFS_DEVICE;
			e->flags |= arg & VFS_NOBLOCK;
			klock_release(&e->lock);
			return 0;
		case F_SETDATA: {
			sVFSNode *n = e->node;
			int res = 0;
			klock_aquire(&waitLock);
			if(e->devNo != VFS_DEV_NO || !IS_DEVICE(n->mode))
				res = -EINVAL;
			else
				res = vfs_device_setReadable(n,(bool)arg);
			klock_release(&waitLock);
			return res;
		}
	}
	return -EINVAL;
}

bool vfs_shouldBlock(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	return !(e->flags & VFS_NOBLOCK);
}

file_t vfs_openPath(pid_t pid,ushort flags,const char *path) {
	inode_t nodeNo;
	file_t file;
	bool created;
	int err;

	/* resolve path */
	err = vfs_node_resolvePath(path,&nodeNo,&created,flags);
	if(err == -EREALPATH) {
		/* unfortunatly we have to check for the process-ids of ata and fs here. because e.g.
		 * if the user tries to mount the device "/realfile" the userspace has no opportunity
		 * to distinguish between virtual and real files. therefore fs will try to open this
		 * path and shoot itself in the foot... */
		if(pid == DISK_PID || pid == FS_PID)
			return -ENOENT;

		/* send msg to fs and wait for reply */
		file = vfs_fsmsgs_openPath(pid,flags,path);
		if(file < 0)
			return file;
	}
	else {
		sVFSNode *node;
		/* handle virtual files */
		if(err < 0)
			return err;

		node = vfs_node_request(nodeNo);
		if(!node)
			return -ENOENT;

		/* if its a device, create the channel-node */
		if(IS_DEVICE(node->mode)) {
			sVFSNode *child;
			/* check if we can access the device */
			if((err = vfs_hasAccess(pid,node,flags)) < 0) {
				vfs_node_release(node);
				return err;
			}
			child = vfs_chan_create(pid,node);
			if(child == NULL) {
				vfs_node_release(node);
				return -ENOMEM;
			}
			nodeNo = vfs_node_getNo(child);
		}
		vfs_node_release(node);

		/* open file */
		file = vfs_openFile(pid,flags,nodeNo,VFS_DEV_NO);
		if(file < 0)
			return file;

		/* if it is a device, call the device open-command; no node-request here because we have
		 * a file for it */
		node = vfs_node_get(nodeNo);
		if(IS_CHANNEL(node->mode)) {
			err = vfs_devmsgs_open(pid,file,node,flags);
			if(err < 0) {
				/* close removes the channeldevice-node, if it is one */
				vfs_closeFile(pid,file);
				return err;
			}
		}
	}

	/* append? */
	if(flags & VFS_APPEND) {
		err = vfs_seek(pid,file,0,SEEK_END);
		if(err < 0) {
			vfs_closeFile(pid,file);
			return err;
		}
	}
	return file;
}

int vfs_openPipe(pid_t pid,file_t *readFile,file_t *writeFile) {
	sVFSNode *node,*pipeNode;
	inode_t nodeNo,pipeNodeNo;
	int err;

	/* resolve pipe-path */
	err = vfs_node_resolvePath("/system/pipe",&nodeNo,NULL,VFS_READ);
	if(err < 0)
		return err;

	/* create pipe */
	node = vfs_node_request(nodeNo);
	pipeNode = vfs_pipe_create(pid,node);
	vfs_node_release(node);
	if(pipeNode == NULL)
		return -ENOMEM;

	pipeNodeNo = vfs_node_getNo(pipeNode);
	/* open file for reading */
	*readFile = vfs_openFile(pid,VFS_READ,pipeNodeNo,VFS_DEV_NO);
	if(*readFile < 0) {
		vfs_node_destroy(pipeNode);
		return *readFile;
	}

	/* open file for writing */
	*writeFile = vfs_openFile(pid,VFS_WRITE,pipeNodeNo,VFS_DEV_NO);
	if(*writeFile < 0) {
		/* closeFile removes the pipenode, too */
		vfs_closeFile(pid,*readFile);
		return *writeFile;
	}
	return 0;
}

file_t vfs_openFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo) {
	sGFTEntry *e;
	sVFSNode *n = NULL;
	file_t f;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DEVICE;

	if(devNo == VFS_DEV_NO) {
		int err;
		n = vfs_node_request(nodeNo);
		if(!n)
			return -ENOENT;
		if((err = vfs_hasAccess(pid,n,flags)) < 0) {
			vfs_node_release(n);
			return err;
		}
	}

	/* determine free file */
	klock_aquire(&gftLock);
	f = vfs_getFreeFile(pid,flags,nodeNo,devNo,n);
	if(f < 0) {
		klock_release(&gftLock);
		if(devNo == VFS_DEV_NO)
			vfs_node_release(n);
		return f;
	}

	e = vfs_getGFTEntry(f);
	klock_aquire(&e->lock);
	/* unused file? */
	if(e->flags == 0) {
		/* count references of virtual nodes */
		if(devNo == VFS_DEV_NO) {
			n->refCount++;
			e->node = n;
			vfs_node_release(n);
		}
		else
			e->node = NULL;
		e->owner = pid;
		e->flags = flags;
		e->refCount = 1;
		e->usageCount = 0;
		e->position = 0;
		e->devNo = devNo;
		e->nodeNo = nodeNo;
	}
	else
		e->refCount++;
	klock_release(&e->lock);
	klock_release(&gftLock);
	return f;
}

static file_t vfs_getFreeFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,sVFSNode *n) {
	const uint userFlags = VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DEVICE;
	file_t i;
	bool isDrvUse = false;
	sGFTEntry *e;
	/* ensure that we don't increment usages of an unused slot */
	assert(flags & (VFS_READ | VFS_WRITE | VFS_MSGS));
	assert(!(flags & ~userFlags));

	if(devNo == VFS_DEV_NO) {
		/* we can add pipes here, too, since every open() to a pipe will get a new node anyway */
		isDrvUse = (n->mode & (MODE_TYPE_CHANNEL | MODE_TYPE_PIPE)) ? true : false;
	}

	/* for devices it doesn't matter whether we use an existing file or a new one, because it is
	 * no problem when multiple threads use it for writing */
	if(!isDrvUse) {
		ushort rwFlags = flags & userFlags;
		for(i = 0; i < FILE_COUNT; i++) {
			e = vfs_getGFTEntry(i);
			/* used slot and same node? */
			if(e->flags != 0) {
				/* same file? */
				if(e->devNo == devNo && e->nodeNo == nodeNo) {
					if(e->owner == pid) {
						/* if the flags are the same we don't need a new file */
						if((e->flags & userFlags) == rwFlags)
							return i;
					}
					/* two procs that want to write at the same time? no! */
					else if(!isDrvUse && (rwFlags & VFS_WRITE) && (e->flags & VFS_WRITE))
						return -EBUSY;
				}
			}
		}
	}

	/* if there is no free slot anymore, extend our dyn-array */
	if(gftFreeList == NULL) {
		i = gftArray.objCount;
		file_t j;
		if(!dyna_extend(&gftArray))
			return -ENFILE;
		/* put all except i on the freelist */
		for(j = i + 1; j < (file_t)gftArray.objCount; j++) {
			e = vfs_getGFTEntry(j);
			e->next = gftFreeList;
			gftFreeList = e;
		}
		return i;
	}

	/* use the first from the freelist */
	e = gftFreeList;
	gftFreeList = gftFreeList->next;
	return dyna_getIndex(&gftArray,e);
}

static void vfs_releaseFile(sGFTEntry *e) {
	klock_aquire(&gftLock);
	e->next = gftFreeList;
	gftFreeList = e;
	klock_release(&gftLock);
}

off_t vfs_tell(A_UNUSED pid_t pid,file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	return e->position;
}

int vfs_stat(pid_t pid,const char *path,USER sFileInfo *info) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_stat(pid,path,info);
	else if(res == 0)
		res = vfs_node_getInfo(nodeNo,info);
	return res;
}

int vfs_fstat(pid_t pid,file_t file,USER sFileInfo *info) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	int res;
	if(e->devNo == VFS_DEV_NO)
		res = vfs_node_getInfo(e->nodeNo,info);
	else
		res = vfs_fsmsgs_istat(pid,e->nodeNo,e->devNo,info);
	return res;
}

int vfs_chmod(pid_t pid,const char *path,mode_t mode) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_chmod(pid,path,mode);
	else if(res == 0)
		res = vfs_node_chmod(pid,nodeNo,mode);
	return res;
}

int vfs_chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == -EREALPATH)
		res = vfs_fsmsgs_chown(pid,path,uid,gid);
	else if(res == 0)
		res = vfs_node_chown(pid,nodeNo,uid,gid);
	return res;
}

off_t vfs_seek(pid_t pid,file_t file,off_t offset,uint whence) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	off_t oldPos,res;

	/* don't lock it during vfs_fsmsgs_istat(). we don't need it in this case because position is
	 * simply set and never restored to oldPos */
	if(e->devNo == VFS_DEV_NO || whence != SEEK_END)
		klock_aquire(&e->lock);

	oldPos = e->position;
	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->seek == NULL) {
			klock_release(&e->lock);
			return -ENOTSUP;
		}
		e->position = n->seek(pid,n,e->position,offset,whence);
	}
	else {
		if(whence == SEEK_END) {
			sFileInfo info;
			res = vfs_fsmsgs_istat(pid,e->nodeNo,e->devNo,&info);
			if(res < 0)
				return res;
			/* can't be < 0, therefore it will always be kept */
			e->position = info.size;
		}
		/* since the fs-device validates the position anyway we can simply set it */
		else if(whence == SEEK_SET)
			e->position = offset;
		else
			e->position += offset;
	}

	/* invalid position? */
	if(e->position < 0) {
		e->position = oldPos;
		res = -EINVAL;
	}
	else
		res = e->position;

	if(e->devNo == VFS_DEV_NO || whence != SEEK_END)
		klock_release(&e->lock);
	return res;
}

ssize_t vfs_readFile(pid_t pid,file_t file,USER void *buffer,size_t count) {
	ssize_t readBytes;
	sGFTEntry *e = vfs_getGFTEntry(file);
	if(!(e->flags & VFS_READ))
		return -EACCES;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->read == NULL)
			return -EACCES;

		/* use the read-handler */
		readBytes = n->read(pid,file,n,buffer,e->position,count);
	}
	else {
		/* query the fs-device to read from the inode */
		readBytes = vfs_fsmsgs_read(pid,e->nodeNo,e->devNo,buffer,e->position,count);
	}

	if(readBytes > 0) {
		klock_aquire(&e->lock);
		e->position += readBytes;
		klock_release(&e->lock);
	}

	if(readBytes > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		/* no lock here because its not critical. we don't make decisions based on it or similar.
		 * its just for statistics. therefore, it doesn't really hurt if we add a bit less in
		 * very very rare cases. */
		p->stats.input += readBytes;
	}
	return readBytes;
}

ssize_t vfs_writeFile(pid_t pid,file_t file,USER const void *buffer,size_t count) {
	ssize_t writtenBytes;
	sGFTEntry *e = vfs_getGFTEntry(file);
	if(!(e->flags & VFS_WRITE))
		return -EACCES;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->write == NULL)
			return -EACCES;

		/* write to the node */
		writtenBytes = n->write(pid,file,n,buffer,e->position,count);
	}
	else {
		/* query the fs-device to write to the inode */
		writtenBytes = vfs_fsmsgs_write(pid,e->nodeNo,e->devNo,buffer,e->position,count);
	}

	if(writtenBytes > 0) {
		klock_aquire(&e->lock);
		e->position += writtenBytes;
		klock_release(&e->lock);
	}

	if(writtenBytes > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		/* no lock; same reason as above */
		p->stats.output += writtenBytes;
	}
	return writtenBytes;
}

ssize_t vfs_sendMsg(pid_t pid,file_t file,msgid_t id,USER const void *data,size_t size) {
	ssize_t err;
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n;

	if(e->devNo != VFS_DEV_NO)
		return -EPERM;
	/* the device-messages (open, read, write, close) are always allowed */
	if(!IS_DEVICE_MSG(id) && !(e->flags & VFS_MSGS))
		return -EACCES;

	/* send the message */
	n = e->node;
	if(!IS_CHANNEL(n->mode))
		return -ENOTSUP;
	/* note the lock-order here! vfs-device does it in the this order, so do we. note also that we
	 * don't lock the channel for device-messages, because vfs-device has already done that. */
	if(!IS_DEVICE_MSG(id))
		vfs_chan_lock(n);
	klock_aquire(&waitLock);
	err = vfs_chan_send(pid,file,n,id,data,size);
	klock_release(&waitLock);
	if(!IS_DEVICE_MSG(id))
		vfs_chan_unlock(n);

	if(err == 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		/* no lock; same reason as above */
		p->stats.output += size;
	}
	return err;
}

ssize_t vfs_receiveMsg(pid_t pid,file_t file,USER msgid_t *id,USER void *data,size_t size) {
	ssize_t err;
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n;

	if(e->devNo != VFS_DEV_NO)
		return -EPERM;

	/* receive the message */
	n = e->node;
	if(!IS_CHANNEL(n->mode))
		return -ENOTSUP;
	err = vfs_chan_receive(pid,file,n,id,data,size);

	if(err > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		/* no lock; same reason as above */
		p->stats.input += err;
	}
	return err;
}

bool vfs_closeFile(pid_t pid,file_t file) {
	bool res;
	sGFTEntry *e = vfs_getGFTEntry(file);
	klock_aquire(&e->lock);
	res = vfs_doCloseFile(pid,file,e);
	klock_release(&e->lock);
	return res;
}

static bool vfs_doCloseFile(pid_t pid,file_t file,sGFTEntry *e) {
	/* decrement references; it may be already zero if we have closed the file previously but
	 * couldn't free it because there was still a user of it. */
	if(e->refCount > 0)
		e->refCount--;

	/* if there are no more references, free the file */
	if(e->refCount == 0) {
		/* if we have used a file-descriptor to get here, the usages are at least 1; otherwise it is
		 * 0, because it is used kernel-intern only and not passed to other "users". */
		if(e->usageCount <= 1) {
			if(e->devNo == VFS_DEV_NO) {
				sVFSNode *n = e->node;
				n->refCount--;
				if(n->close)
					n->close(pid,file,n);
			}
			/* vfs_fsmsgs_close won't cause a context-switch; therefore we can keep the lock */
			else
				vfs_fsmsgs_close(pid,e->nodeNo,e->devNo);

			/* mark unused */
			e->flags = 0;
			vfs_releaseFile(e);
			return true;
		}
	}
	return false;
}

static bool vfs_hasMsg(sVFSNode *node) {
	return IS_CHANNEL(node->mode) && vfs_chan_hasReply(node);
}

static bool vfs_hasData(sVFSNode *node) {
	return IS_DEVICE(node->parent->mode) && vfs_device_isReadable(node->parent);
}

static bool vfs_hasWork(sVFSNode *node) {
	return IS_DEVICE(node->mode) && vfs_device_hasWork(node);
}

int vfs_waitFor(sWaitObject *objects,size_t objCount,time_t maxWaitTime,bool block,
		pid_t pid,ulong ident) {
	sThread *t = thread_getRunning();
	size_t i;
	bool isFirstWait = true;
	int res;

	/* transform the files into vfs-nodes */
	for(i = 0; i < objCount; i++) {
		if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			file_t file = (file_t)objects[i].object;
			sGFTEntry *e = vfs_getGFTEntry(file);
			if(e->devNo != VFS_DEV_NO)
				return -EPERM;
			objects[i].object = (evobj_t)e->node;
		}
	}

	while(true) {
		/* we have to lock this region to ensure that if we've found out that we can sleep, no one
		 * sends us an event before we've finished the ev_waitObjects(). otherwise, it would be
		 * possible that we never wake up again, because we have missed the event and get no other
		 * one. */
		klock_aquire(&waitLock);
		/* check whether we can wait */
		for(i = 0; i < objCount; i++) {
			if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
				sVFSNode *n = (sVFSNode*)objects[i].object;
				if(n->name == NULL) {
					res = -EDESTROYED;
					goto error;
				}
				if((objects[i].events & EV_CLIENT) && vfs_hasWork(n))
					goto noWait;
				else if((objects[i].events & EV_RECEIVED_MSG) && vfs_hasMsg(n))
					goto noWait;
				else if((objects[i].events & EV_DATA_READABLE) && vfs_hasData(n))
					goto noWait;
			}
		}

		if(!block) {
			klock_release(&waitLock);
			return -EWOULDBLOCK;
		}

		/* wait */
		if(!ev_waitObjects(t,objects,objCount)) {
			klock_release(&waitLock);
			res = -ENOMEM;
			goto error;
		}
		if(pid != KERNEL_PID)
			lock_release(pid,ident);
		if(isFirstWait && maxWaitTime != 0)
			timer_sleepFor(t->tid,maxWaitTime,true);
		klock_release(&waitLock);

		thread_switch();
		if(sig_hasSignalFor(t->tid)) {
			res = -EINTR;
			goto error;
		}
		/* if we're waiting for other events, too, we have to wake up */
		for(i = 0; i < objCount; i++) {
			if((objects[i].events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)))
				goto done;
		}
		isFirstWait = false;
	}

noWait:
	if(pid != KERNEL_PID)
		lock_release(pid,ident);
	klock_release(&waitLock);
done:
	res = 0;
error:
	if(maxWaitTime != 0)
		timer_removeThread(t->tid);
	return res;
}

static inode_t vfs_doGetClient(const file_t *files,size_t count,size_t *index) {
	sVFSNode *match = NULL;
	size_t i;
	bool retry,cont = true;
	do {
		retry = false;
		for(i = 0; cont && i < count; i++) {
			sGFTEntry *e = vfs_getGFTEntry(files[i]);
			sVFSNode *client,*node = e->node;
			if(e->devNo != VFS_DEV_NO)
				return -EPERM;

			if(!IS_DEVICE(node->mode))
				return -EPERM;

			client = vfs_device_getWork(node,&cont,&retry);
			if(client) {
				if(index)
					*index = i;
				if(cont)
					match = client;
				else {
					vfs_chan_setUsed(client,true);
					return vfs_node_getNo(client);
				}
			}
		}
		/* if we have a match, use this one */
		if(match) {
			vfs_chan_setUsed(match,true);
			return vfs_node_getNo(match);
		}
		/* if not and we've skipped a client, try another time */
	}
	while(retry);
	return -ENOCLIENT;
}

inode_t vfs_getClient(const file_t *files,size_t count,size_t *index,uint flags) {
	sWaitObject waits[MAX_GETWORK_DEVICES];
	sThread *t = thread_getRunning();
	bool inited = false;
	inode_t clientNo;
	while(true) {
		klock_aquire(&waitLock);
		clientNo = vfs_doGetClient(files,count,index);
		if(clientNo != -ENOCLIENT) {
			klock_release(&waitLock);
			break;
		}

		/* if we shouldn't block, stop here */
		if(flags & GW_NOBLOCK) {
			klock_release(&waitLock);
			break;
		}

		/* build wait-objects */
		if(!inited) {
			size_t i;
			for(i = 0; i < count; i++) {
				sGFTEntry *e = vfs_getGFTEntry(files[i]);
				waits[i].events = EV_CLIENT;
				waits[i].object = (evobj_t)e->node;
			}
			inited = true;
		}

		/* wait for a client (accept signals) */
		ev_waitObjects(t,waits,count);
		klock_release(&waitLock);

		thread_switch();
		if(sig_hasSignalFor(t->tid)) {
			clientNo = -EINTR;
			break;
		}
	}
	return clientNo;
}

inode_t vfs_getClientId(A_UNUSED pid_t pid,file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n = e->node;
	if(e->devNo != VFS_DEV_NO || !IS_CHANNEL(n->mode))
		return -EPERM;
	return e->nodeNo;
}

file_t vfs_openClient(pid_t pid,file_t file,inode_t clientId) {
	bool isValid;
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n;

	/* search for the client */
	n = vfs_node_openDir(e->node,true,&isValid);
	if(isValid) {
		while(n != NULL) {
			if(vfs_node_getNo(n) == clientId)
				break;
			n = n->next;
		}
	}
	vfs_node_closeDir(e->node,true);
	if(n == NULL)
		return -ENOENT;

	/* open file */
	return vfs_openFile(pid,VFS_MSGS | VFS_DEVICE,vfs_node_getNo(n),VFS_DEV_NO);
}

int vfs_mount(pid_t pid,const char *device,const char *path,uint type) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != -EREALPATH)
		return -EPERM;
	return vfs_fsmsgs_mount(pid,device,path,type);
}

int vfs_unmount(pid_t pid,const char *path) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != -EREALPATH)
		return -EPERM;
	return vfs_fsmsgs_unmount(pid,path);
}

int vfs_sync(pid_t pid) {
	return vfs_fsmsgs_sync(pid);
}

int vfs_link(pid_t pid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy,backup;
	size_t len;
	inode_t oldIno,newIno;
	sVFSNode *dir,*target;
	int res,oldRes,newRes;
	/* first check whether it is a realpath */
	oldRes = vfs_node_resolvePath(oldPath,&oldIno,NULL,VFS_READ);
	newRes = vfs_node_resolvePath(newPath,&newIno,NULL,VFS_WRITE);
	if(oldRes == -EREALPATH) {
		if(newRes != -EREALPATH)
			return -EXDEV;
		return vfs_fsmsgs_link(pid,oldPath,newPath);
	}
	if(oldRes < 0)
		return oldRes;
	if(newRes >= 0)
		return -EEXIST;

	/* TODO prevent recursion? */
	/* TODO check access-rights */

	/* copy path because we have to change it */
	len = strlen(newPath);
	strcpy(newPathCpy,newPath);
	/* check whether the directory exists */
	name = vfs_node_basename((char*)newPathCpy,&len);
	backup = *name;
	vfs_node_dirname((char*)newPathCpy,len);
	newRes = vfs_node_resolvePath(newPathCpy,&newIno,NULL,VFS_WRITE);
	if(newRes < 0)
		return -ENOENT;

	/* links to directories not allowed */
	target = vfs_node_request(oldIno);
	if(!target)
		return -ENOENT;
	if(S_ISDIR(target->mode)) {
		res = -EISDIR;
		goto errorTarget;
	}

	/* make copy of name */
	*name = backup;
	len = strlen(name);
	namecpy = cache_alloc(len + 1);
	if(namecpy == NULL) {
		res = -ENOMEM;
		goto errorTarget;
	}
	strcpy(namecpy,name);
	/* file exists? */
	if(vfs_node_findInDirOf(newIno,namecpy,len) != NULL) {
		res = -EEXIST;
		goto errorName;
	}
	/* now create link */
	dir = vfs_node_request(newIno);
	if(!dir) {
		res = -ENOENT;
		goto errorName;
	}
	if(vfs_link_create(pid,dir,namecpy,target) == NULL) {
		res = -ENOMEM;
		goto errorDir;
	}
	vfs_node_release(dir);
	return 0;

errorDir:
	vfs_node_release(dir);
errorName:
	cache_free(namecpy);
errorTarget:
	vfs_node_release(target);
	return res;
}

int vfs_unlink(pid_t pid,const char *path) {
	int res;
	inode_t ino;
	sVFSNode *n;
	res = vfs_node_resolvePath(path,&ino,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == -EREALPATH)
		return vfs_fsmsgs_unlink(pid,path);
	if(res < 0)
		return -ENOENT;
	/* TODO check access-rights */
	n = vfs_node_request(ino);
	if(!n)
		return -ENOENT;
	if(!S_ISREG(n->mode) && !S_ISLNK(n->mode)) {
		vfs_node_release(n);
		return -EPERM;
	}
	vfs_node_release(n);
	vfs_node_destroy(n);
	return 0;
}

int vfs_mkdir(pid_t pid,const char *path) {
	char pathCpy[MAX_PATH_LEN + 1];
	char *name,*namecpy;
	char backup;
	int res;
	size_t len = strlen(path);
	inode_t inodeNo;
	sVFSNode *node,*child;

	/* copy path because we'll change it */
	if(len >= MAX_PATH_LEN)
		return -ENAMETOOLONG;
	strcpy(pathCpy,path);

	/* extract name and directory */
	name = vfs_node_basename(pathCpy,&len);
	backup = *name;
	vfs_node_dirname(pathCpy,len);

	/* get the parent-directory */
	res = vfs_node_resolvePath(pathCpy,&inodeNo,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == -EREALPATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		/* let fs handle the request */
		return vfs_fsmsgs_mkdir(pid,path);
	}
	if(res < 0)
		return res;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	namecpy = cache_alloc(len + 1);
	if(namecpy == NULL)
		return -ENOMEM;
	strcpy(namecpy,name);
	/* does it exist? */
	if(vfs_node_findInDirOf(inodeNo,namecpy,len) != NULL) {
		cache_free(namecpy);
		return -EEXIST;
	}
	/* TODO check access-rights */
	/* create dir */
	node = vfs_node_request(inodeNo);
	if(!node) {
		cache_free(namecpy);
		return -ENOENT;
	}
	child = vfs_dir_create(pid,node,namecpy);
	if(child == NULL) {
		vfs_node_release(node);
		cache_free(namecpy);
		return -ENOMEM;
	}
	vfs_node_release(node);
	return 0;
}

int vfs_rmdir(pid_t pid,const char *path) {
	int res;
	sVFSNode *node;
	inode_t inodeNo;
	res = vfs_node_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == -EREALPATH)
		return vfs_fsmsgs_rmdir(pid,path);
	if(res < 0)
		return -ENOENT;

	/* TODO check access-rights */
	node = vfs_node_request(inodeNo);
	if(!node)
		return -ENOENT;
	if(!S_ISDIR(node->mode)) {
		vfs_node_release(node);
		return -ENOTDIR;
	}
	vfs_node_release(node);
	vfs_node_destroy(node);
	return 0;
}

file_t vfs_createdev(pid_t pid,char *path,uint type,uint ops) {
	sVFSNode *dir,*srv;
	size_t len;
	char *name;
	file_t res;
	inode_t nodeNo;
	int err;

	/* get name */
	len = strlen(path);
	name = vfs_node_basename(path,&len);
	name = strdup(name);
	if(!name)
		return -ENOMEM;

	/* check whether the directory exists */
	vfs_node_dirname(path,len);
	err = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	/* this includes -EREALPATH since devices have to be created in the VFS */
	if(err < 0)
		goto errorName;

	/* ensure its a directory */
	dir = vfs_node_request(nodeNo);
	if(!dir)
		goto errorName;
	if(!S_ISDIR(dir->mode))
		goto errorDir;

	/* check whether the device does already exist */
	if(vfs_node_findInDir(dir,name,strlen(name)) != NULL) {
		err = -EEXIST;
		goto errorDir;
	}

	/* create node */
	srv = vfs_device_create(pid,dir,name,type,ops);
	if(!srv) {
		err = -ENOMEM;
		goto errorDir;
	}
	res = vfs_openFile(pid,VFS_MSGS | VFS_DEVICE,vfs_node_getNo(srv),VFS_DEV_NO);
	if(res < 0) {
		err = res;
		goto errDevice;
	}
	vfs_node_release(dir);
	return res;

errDevice:
	vfs_node_destroy(srv);
errorDir:
	vfs_node_release(dir);
errorName:
	cache_free(name);
	return err;
}

inode_t vfs_createProcess(pid_t pid) {
	char *name;
	sVFSNode *proc = procsNode;
	sVFSNode *n;
	sVFSNode *dir,*tdir;
	bool isValid;
	int res = -ENOMEM;

	/* build name */
	name = (char*)cache_alloc(12);
	if(name == NULL)
		return -ENOMEM;

	itoa(name,12,pid);

	/* go to last entry */
	n = vfs_node_openDir(proc,true,&isValid);
	if(!isValid) {
		vfs_node_closeDir(proc,true);
		return -EDESTROYED;
	}
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0) {
			vfs_node_closeDir(proc,true);
			res = -EEXIST;
			goto errorName;
		}
		n = n->next;
	}
	vfs_node_closeDir(proc,true);

	/* create dir */
	dir = vfs_dir_create(KERNEL_PID,proc,name);
	if(dir == NULL)
		goto errorName;

	dir = vfs_node_request(vfs_node_getNo(dir));
	if(!dir)
		goto errorName;
	/* create process-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",vfs_info_procReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create virt-mem-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"virtmem",vfs_info_virtMemReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create regions-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"regions",vfs_info_regionsReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create maps-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"maps",vfs_info_mapsReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create threads-dir */
	tdir = vfs_dir_create(KERNEL_PID,dir,(char*)"threads");
	if(tdir == NULL)
		goto errorDir;

	vfs_node_release(dir);
	return vfs_node_getNo(tdir);

errorDir:
	vfs_node_release(dir);
	vfs_node_destroy(dir);
errorName:
	cache_free(name);
	return res;
}

void vfs_removeProcess(pid_t pid) {
	/* remove from /system/processes */
	const sProc *p = proc_getByPid(pid);
	sVFSNode *node = vfs_node_get(p->threadDir);
	vfs_node_destroy(node->parent);
	vfs_fsmsgs_removeProc(pid);
}

bool vfs_createThread(tid_t tid) {
	char *name;
	sVFSNode *n,*dir;
	const sThread *t = thread_getById(tid);

	/* build name */
	name = (char*)cache_alloc(12);
	if(name == NULL)
		return false;
	itoa(name,12,tid);

	/* create dir */
	n = vfs_node_get(t->proc->threadDir);
	dir = vfs_dir_create(KERNEL_PID,n,name);
	if(dir == NULL)
		goto errorDir;

	/* create info-node */
	dir = vfs_node_request(vfs_node_getNo(dir));
	if(!dir)
		goto errorDir;
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",vfs_info_threadReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;

	/* create trace-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"trace",vfs_info_traceReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;
	vfs_node_release(dir);
	return true;

errorInfo:
	vfs_node_release(dir);
	vfs_node_destroy(dir);
errorDir:
	cache_free(name);
	return false;
}

void vfs_removeThread(tid_t tid) {
	sThread *t = thread_getById(tid);
	sVFSNode *n,*dir;
	char *name;
	bool isValid;

	/* build name */
	name = (char*)cache_alloc(12);
	if(name == NULL)
		return;
	itoa(name,12,tid);

	/* search for thread-node and remove it */
	dir = vfs_node_get(t->proc->threadDir);
	n = vfs_node_openDir(dir,true,&isValid);
	if(!isValid) {
		vfs_node_closeDir(dir,true);
		return;
	}
	while(n != NULL) {
		if(strcmp(n->name,name) == 0) {
			vfs_node_closeDir(dir,true);
			vfs_node_destroy(n);
			break;
		}
		n = n->next;
	}
	cache_free(name);
	vfs_req_freeAllOf(t);
}

size_t vfs_dbg_getGFTEntryCount(void) {
	file_t i;
	size_t count = 0;
	for(i = 0; i < FILE_COUNT; i++) {
		if(vfs_getGFTEntry(i)->flags != 0)
			count++;
	}
	return count;
}

void vfs_printMsgs(void) {
	bool isValid;
	sVFSNode *drv = vfs_node_openDir(devNode,true,&isValid);
	if(isValid) {
		vid_printf("Messages:\n");
		while(drv != NULL) {
			if(IS_DEVICE(drv->mode))
				vfs_device_print(drv);
			drv = drv->next;
		}
	}
	vfs_node_closeDir(devNode,true);
}

void vfs_printFile(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	vid_printf("%3d [ %2u refs, %2u uses (%u:%u",file,e->refCount,e->usageCount,e->devNo,e->nodeNo);
	if(e->devNo == VFS_DEV_NO && vfs_node_isValid(e->nodeNo))
		vid_printf(":%s)",vfs_node_getPath(e->nodeNo));
	else
		vid_printf(")");
	vid_printf(" ]");
}

void vfs_printGFT(void) {
	file_t i;
	sGFTEntry *e;
	vid_printf("Global File Table:\n");
	for(i = 0; i < FILE_COUNT; i++) {
		e = vfs_getGFTEntry(i);
		if(e->flags != 0) {
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tflags: ");
			if(e->flags & VFS_READ)
				vid_printf("READ ");
			if(e->flags & VFS_WRITE)
				vid_printf("WRITE ");
			if(e->flags & VFS_NOBLOCK)
				vid_printf("NOBLOCK ");
			if(e->flags & VFS_DEVICE)
				vid_printf("DEVICE ");
			if(e->flags & VFS_MSGS)
				vid_printf("MSGS ");
			vid_printf("\n");
			vid_printf("\t\tnodeNo: %d\n",e->nodeNo);
			vid_printf("\t\tdevNo: %d\n",e->devNo);
			vid_printf("\t\tpos: %Od\n",e->position);
			vid_printf("\t\trefCount: %d\n",e->refCount);
			if(e->owner == KERNEL_PID)
				vid_printf("\t\towner: %d (kernel)\n",e->owner);
			else {
				const sProc *p = proc_getByPid(e->owner);
				vid_printf("\t\towner: %d:%s\n",e->owner,p ? p->command : "???");
			}
			if(e->devNo == VFS_DEV_NO) {
				sVFSNode *n = e->node;
				if(n->name == NULL)
					vid_printf("\t\tFile: <destroyed>\n");
				else if(IS_CHANNEL(n->mode))
					vid_printf("\t\tChannel: %s @ %s\n",n->name,n->parent->name);
				else
					vid_printf("\t\tFile: '%s'\n",vfs_node_getPath(vfs_node_getNo(n)));
			}
		}
	}
}
