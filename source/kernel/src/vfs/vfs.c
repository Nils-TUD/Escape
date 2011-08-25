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
#include <sys/vfs/real.h>
#include <sys/vfs/info.h>
#include <sys/vfs/request.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/file.h>
#include <sys/vfs/dir.h>
#include <sys/vfs/link.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/pipe.h>
#include <sys/vfs/server.h>
#include <sys/task/proc.h>
#include <sys/task/groups.h>
#include <sys/task/event.h>
#include <sys/task/lock.h>
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

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
	vfs_drv_init();
	vfs_real_init();
}

int vfs_hasAccess(pid_t pid,sVFSNode *n,ushort flags) {
	const sProc *p;
	uint mode;
	if(n->name == NULL)
		return ERR_INVALID_FILE;
	/* kernel is allmighty :P */
	if(pid == KERNEL_PID)
		return 0;

	p = proc_getByPid(pid);
	if(p == NULL)
		return ERR_INVALID_PID;
	/* root is (nearly) allmighty as well */
	if(p->euid == ROOT_UID) {
		/* root has exec-permission if at least one has exec-permission */
		if(flags & VFS_EXEC)
			return (n->mode & MODE_EXEC) ? 0 : ERR_NO_EXEC_PERM;
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
		return ERR_NO_READ_PERM;
	if((flags & VFS_WRITE) && !(mode & MODE_WRITE))
		return ERR_NO_WRITE_PERM;
	if((flags & VFS_EXEC) && !(mode & MODE_EXEC))
		return ERR_NO_EXEC_PERM;
	return 0;
}

static sGFTEntry *vfs_getGFTEntry(file_t file) {
	return (sGFTEntry*)dyna_getObj(&gftArray,file);
}

bool vfs_isDriver(file_t file) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	return e->flags & VFS_DRIVER;
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

int vfs_fcntl(pid_t pid,file_t file,uint cmd,int arg) {
	UNUSED(pid);
	sGFTEntry *e = vfs_getGFTEntry(file);
	switch(cmd) {
		case F_GETACCESS:
			return e->flags & (VFS_READ | VFS_WRITE | VFS_MSGS);
		case F_GETFL:
			return e->flags & VFS_NOBLOCK;
		case F_SETFL:
			klock_aquire(&e->lock);
			e->flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_CREATE | VFS_DRIVER;
			e->flags |= arg & VFS_NOBLOCK;
			klock_release(&e->lock);
			return 0;
		case F_SETDATA: {
			sVFSNode *n = e->node;
			int res = 0;
			klock_aquire(&waitLock);
			if(e->devNo != VFS_DEV_NO || !IS_DRIVER(n->mode))
				res = ERR_INVALID_ARGS;
			else
				res = vfs_server_setReadable(n,(bool)arg);
			klock_release(&waitLock);
			return res;
		}
	}
	return ERR_INVALID_ARGS;
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
	if(err == ERR_REAL_PATH) {
		/* unfortunatly we have to check for the process-ids of ata and fs here. because e.g.
		 * if the user tries to mount the device "/realfile" the userspace has no opportunity
		 * to distinguish between virtual and real files. therefore fs will try to open this
		 * path and shoot itself in the foot... */
		if(pid == DISK_PID || pid == FS_PID)
			return ERR_PATH_NOT_FOUND;

		/* send msg to fs and wait for reply */
		file = vfs_real_openPath(pid,flags,path);
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
			return ERR_INVALID_INODENO;

		/* if its a driver, create the channel-node */
		if(IS_DRIVER(node->mode)) {
			sVFSNode *child;
			/* check if we can access the driver */
			if((err = vfs_hasAccess(pid,node,flags)) < 0) {
				vfs_node_release(node);
				return err;
			}
			child = vfs_chan_create(pid,node);
			if(child == NULL) {
				vfs_node_release(node);
				return ERR_NOT_ENOUGH_MEM;
			}
			nodeNo = vfs_node_getNo(child);
		}
		vfs_node_release(node);

		/* open file */
		file = vfs_openFile(pid,flags,nodeNo,VFS_DEV_NO);
		if(file < 0)
			return file;

		/* if it is a driver, call the driver open-command; no node-request here because we have
		 * a file for it */
		node = vfs_node_get(nodeNo);
		if(IS_CHANNEL(node->mode)) {
			err = vfs_drv_open(pid,file,node,flags);
			if(err < 0) {
				/* close removes the driver-usage-node, if it is one */
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
		return ERR_NOT_ENOUGH_MEM;

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
	flags &= VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DRIVER;

	if(devNo == VFS_DEV_NO) {
		int err;
		n = vfs_node_request(nodeNo);
		if(!n)
			return ERR_INVALID_INODENO;
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
	const uint userFlags = VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOBLOCK | VFS_DRIVER;
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

	/* for drivers it doesn't matter whether we use an existing file or a new one, because it is
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
						return ERR_FILE_IN_USE;
				}
			}
		}
	}

	/* if there is no free slot anymore, extend our dyn-array */
	if(gftFreeList == NULL) {
		i = gftArray.objCount;
		file_t j;
		if(!dyna_extend(&gftArray))
			return ERR_NO_FREE_FILE;
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

off_t vfs_tell(pid_t pid,file_t file) {
	UNUSED(pid);
	sGFTEntry *e = vfs_getGFTEntry(file);
	return e->position;
}

int vfs_stat(pid_t pid,const char *path,USER sFileInfo *info) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH)
		res = vfs_real_stat(pid,path,info);
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
		res = vfs_real_istat(pid,e->nodeNo,e->devNo,info);
	return res;
}

int vfs_chmod(pid_t pid,const char *path,mode_t mode) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH)
		res = vfs_real_chmod(pid,path,mode);
	else if(res == 0)
		res = vfs_node_chmod(pid,nodeNo,mode);
	return res;
}

int vfs_chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	inode_t nodeNo;
	int res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH)
		res = vfs_real_chown(pid,path,uid,gid);
	else if(res == 0)
		res = vfs_node_chown(pid,nodeNo,uid,gid);
	return res;
}

off_t vfs_seek(pid_t pid,file_t file,off_t offset,uint whence) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	off_t oldPos,res;

	/* don't lock it during vfs_real_istat(). we don't need it in this case because position is
	 * simply set and never restored to oldPos */
	if(e->devNo == VFS_DEV_NO || whence != SEEK_END)
		klock_aquire(&e->lock);

	oldPos = e->position;
	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->seek == NULL) {
			klock_release(&e->lock);
			return ERR_UNSUPPORTED_OP;
		}
		e->position = n->seek(pid,n,e->position,offset,whence);
	}
	else {
		if(whence == SEEK_END) {
			sFileInfo info;
			res = vfs_real_istat(pid,e->nodeNo,e->devNo,&info);
			if(res < 0)
				return res;
			/* can't be < 0, therefore it will always be kept */
			e->position = info.size;
		}
		/* since the fs-driver validates the position anyway we can simply set it */
		else if(whence == SEEK_SET)
			e->position = offset;
		else
			e->position += offset;
	}

	/* invalid position? */
	if(e->position < 0) {
		e->position = oldPos;
		res = ERR_INVALID_ARGS;
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
		return ERR_NO_READ_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->read == NULL)
			return ERR_NO_READ_PERM;

		/* use the read-handler */
		readBytes = n->read(pid,file,n,buffer,e->position,count);
	}
	else {
		/* query the fs-driver to read from the inode */
		readBytes = vfs_real_read(pid,e->nodeNo,e->devNo,buffer,e->position,count);
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
		return ERR_NO_WRITE_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = e->node;
		if(n->write == NULL)
			return ERR_NO_WRITE_PERM;

		/* write to the node */
		writtenBytes = n->write(pid,file,n,buffer,e->position,count);
	}
	else {
		/* query the fs-driver to write to the inode */
		writtenBytes = vfs_real_write(pid,e->nodeNo,e->devNo,buffer,e->position,count);
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
		return ERR_INVALID_FILE;
	/* the driver-messages (open, read, write, close) are always allowed */
	if(!IS_DRIVER_MSG(id) && !(e->flags & VFS_MSGS))
		return ERR_NO_EXEC_PERM;

	/* send the message */
	n = e->node;
	if(!IS_CHANNEL(n->mode))
		return ERR_UNSUPPORTED_OP;
	/* note the lock-order here! vfs-driver does it in the this order, so do we. note also that we
	 * don't lock the channel for driver-messages, because vfs-driver has already done that. */
	if(!IS_DRIVER_MSG(id))
		vfs_chan_lock(n);
	klock_aquire(&waitLock);
	err = vfs_chan_send(pid,file,n,id,data,size);
	klock_release(&waitLock);
	if(!IS_DRIVER_MSG(id))
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
		return ERR_INVALID_FILE;

	/* receive the message */
	n = e->node;
	if(!IS_CHANNEL(n->mode))
		return ERR_UNSUPPORTED_OP;
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
				if(n->name != NULL) {
					n->refCount--;
					if(n->close)
						n->close(pid,file,n);
				}
			}
			/* vfs_real_close won't cause a context-switch; therefore we can keep the lock */
			else
				vfs_real_close(pid,e->nodeNo,e->devNo);

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
	return IS_DRIVER(node->parent->mode) && vfs_server_isReadable(node->parent);
}

static bool vfs_hasWork(sVFSNode *node) {
	return IS_DRIVER(node->mode) && vfs_server_hasWork(node);
}

int vfs_waitFor(sWaitObject *objects,size_t objCount,bool block,pid_t pid,ulong ident) {
	sThread *t = thread_getRunning();
	size_t i;

	/* transform the files into vfs-nodes */
	for(i = 0; i < objCount; i++) {
		if(objects[i].events & (EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)) {
			file_t file = (file_t)objects[i].object;
			sGFTEntry *e = vfs_getGFTEntry(file);
			if(e->devNo != VFS_DEV_NO)
				return ERR_INVALID_ARGS;
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
			return ERR_WOULD_BLOCK;
		}

		/* wait */
		if(!ev_waitObjects(t,objects,objCount)) {
			klock_release(&waitLock);
			return ERR_NOT_ENOUGH_MEM;
		}
		if(pid != KERNEL_PID)
			lock_release(pid,ident);
		klock_release(&waitLock);

		thread_switch();
		if(sig_hasSignalFor(t->tid))
			return ERR_INTERRUPTED;
		/* if we're waiting for other events, too, we have to wake up */
		for(i = 0; i < objCount; i++) {
			if((objects[i].events & ~(EV_CLIENT | EV_RECEIVED_MSG | EV_DATA_READABLE)))
				return 0;
		}
	}

noWait:
	if(pid != KERNEL_PID)
		lock_release(pid,ident);
	klock_release(&waitLock);
	return 0;
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
				return ERR_INVALID_FILE;

			if(!IS_DRIVER(node->mode))
				return ERR_NOT_OWN_DRIVER;

			client = vfs_server_getWork(node,&cont,&retry);
			if(client) {
				if(index)
					*index = i;
				if(cont)
					match = client;
				else
					return vfs_node_getNo(client);
			}
		}
		/* if we have a match, use this one */
		if(match)
			return vfs_node_getNo(match);
		/* if not and we've skipped a client, try another time */
	}
	while(retry);
	return ERR_NO_CLIENT_WAITING;
}

inode_t vfs_getClient(const file_t *files,size_t count,size_t *index,uint flags) {
	sWaitObject waits[MAX_GETWORK_DRIVERS];
	sThread *t = thread_getRunning();
	bool inited = false;
	inode_t clientNo;
	while(true) {
		klock_aquire(&waitLock);
		clientNo = vfs_doGetClient(files,count,index);
		if(clientNo != ERR_NO_CLIENT_WAITING) {
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
			clientNo = ERR_INTERRUPTED;
			break;
		}
	}
	return clientNo;
}

inode_t vfs_getClientId(pid_t pid,file_t file) {
	UNUSED(pid);
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n = e->node;
	if(e->devNo != VFS_DEV_NO || !IS_CHANNEL(n->mode))
		return ERR_INVALID_FILE;
	return e->nodeNo;
}

file_t vfs_openClient(pid_t pid,file_t file,inode_t clientId) {
	sGFTEntry *e = vfs_getGFTEntry(file);
	sVFSNode *n;

	/* search for the client */
	n = vfs_node_openDir(e->node,true);
	while(n != NULL) {
		if(vfs_node_getNo(n) == clientId)
			break;
		n = n->next;
	}
	vfs_node_closeDir(e->node,true);
	if(n == NULL)
		return ERR_PATH_NOT_FOUND;

	/* open file */
	return vfs_openFile(pid,VFS_MSGS | VFS_DRIVER,vfs_node_getNo(n),VFS_DEV_NO);
}

int vfs_mount(pid_t pid,const char *device,const char *path,uint type) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		return ERR_MOUNT_VIRT_PATH;
	return vfs_real_mount(pid,device,path,type);
}

int vfs_unmount(pid_t pid,const char *path) {
	inode_t ino;
	if(vfs_node_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		return ERR_MOUNT_VIRT_PATH;
	return vfs_real_unmount(pid,path);
}

int vfs_sync(pid_t pid) {
	return vfs_real_sync(pid);
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
	if(oldRes == ERR_REAL_PATH) {
		if(newRes != ERR_REAL_PATH)
			return ERR_LINK_DEVICE;
		return vfs_real_link(pid,oldPath,newPath);
	}
	if(oldRes < 0)
		return oldRes;
	if(newRes >= 0)
		return ERR_FILE_EXISTS;

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
		return ERR_PATH_NOT_FOUND;

	/* links to directories not allowed */
	target = vfs_node_request(oldIno);
	if(!target)
		return ERR_INVALID_INODENO;
	if(S_ISDIR(target->mode)) {
		res = ERR_IS_DIR;
		goto errorTarget;
	}

	/* make copy of name */
	*name = backup;
	len = strlen(name);
	namecpy = cache_alloc(len + 1);
	if(namecpy == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorTarget;
	}
	strcpy(namecpy,name);
	/* file exists? */
	if(vfs_node_findInDir(newIno,namecpy,len) != NULL) {
		res = ERR_FILE_EXISTS;
		goto errorName;
	}
	/* now create link */
	dir = vfs_node_request(newIno);
	if(!dir) {
		res = ERR_INVALID_INODENO;
		goto errorName;
	}
	if(vfs_link_create(pid,dir,namecpy,target) == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
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
	if(res == ERR_REAL_PATH)
		return vfs_real_unlink(pid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;
	/* TODO check access-rights */
	n = vfs_node_request(ino);
	if(!n)
		return ERR_INVALID_INODENO;
	if(!S_ISREG(n->mode) && !S_ISLNK(n->mode)) {
		vfs_node_release(n);
		return ERR_NO_FILE_OR_LINK;
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
		return ERR_INVALID_PATH;
	strcpy(pathCpy,path);

	/* extract name and directory */
	name = vfs_node_basename(pathCpy,&len);
	backup = *name;
	vfs_node_dirname(pathCpy,len);

	/* get the parent-directory */
	res = vfs_node_resolvePath(pathCpy,&inodeNo,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == ERR_REAL_PATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		/* let fs handle the request */
		return vfs_real_mkdir(pid,path);
	}
	if(res < 0)
		return res;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	namecpy = cache_alloc(len + 1);
	if(namecpy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(namecpy,name);
	/* does it exist? */
	if(vfs_node_findInDir(inodeNo,namecpy,len) != NULL) {
		cache_free(namecpy);
		return ERR_FILE_EXISTS;
	}
	/* TODO check access-rights */
	/* create dir */
	node = vfs_node_request(inodeNo);
	if(!node) {
		cache_free(namecpy);
		return ERR_INVALID_INODENO;
	}
	child = vfs_dir_create(pid,node,namecpy);
	if(child == NULL) {
		vfs_node_release(node);
		cache_free(namecpy);
		return ERR_NOT_ENOUGH_MEM;
	}
	vfs_node_release(node);
	return 0;
}

int vfs_rmdir(pid_t pid,const char *path) {
	int res;
	sVFSNode *node;
	inode_t inodeNo;
	res = vfs_node_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == ERR_REAL_PATH)
		return vfs_real_rmdir(pid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;

	/* TODO check access-rights */
	node = vfs_node_request(inodeNo);
	if(!node)
		return ERR_INVALID_INODENO;
	if(!S_ISDIR(node->mode)) {
		vfs_node_release(node);
		return ERR_NO_DIRECTORY;
	}
	vfs_node_release(node);
	vfs_node_destroy(node);
	return 0;
}

file_t vfs_createDriver(pid_t pid,const char *name,uint flags) {
	sVFSNode *drv = devNode;
	sVFSNode *n,*srv;
	size_t len;
	char *hname;
	file_t res;
	vassert(name != NULL,"name == NULL");

	/* TODO check permissions */

	/* we don't want to have exotic driver-names */
	if((len = strlen(name)) == 0 || !isalnumstr(name))
		return ERR_INV_DRIVER_NAME;

	n = vfs_node_openDir(drv,true);
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0) {
			vfs_node_closeDir(drv,true);
			return ERR_DRIVER_EXISTS;
		}
		n = n->next;
	}

	/* copy name to kernel-heap */
	hname = (char*)cache_alloc(len + 1);
	if(hname == NULL)
		goto errorDir;
	strncpy(hname,name,len);
	hname[len] = '\0';

	/* create node */
	srv = vfs_server_create(pid,drv,hname,flags);
	if(!srv)
		goto errorName;
	res = vfs_openFile(pid,VFS_MSGS | VFS_DRIVER,vfs_node_getNo(srv),VFS_DEV_NO);
	if(res < 0)
		goto errorServer;
	vfs_node_closeDir(drv,true);
	return res;

errorServer:
	vfs_node_destroy(n);
errorName:
	cache_free(hname);
errorDir:
	vfs_node_closeDir(drv,true);
	return ERR_NOT_ENOUGH_MEM;
}

inode_t vfs_createProcess(pid_t pid) {
	char *name;
	sVFSNode *proc = procsNode;
	sVFSNode *n;
	sVFSNode *dir,*tdir;
	int res = ERR_NOT_ENOUGH_MEM;

	/* build name */
	name = (char*)cache_alloc(12);
	if(name == NULL)
		return ERR_NOT_ENOUGH_MEM;

	itoa(name,12,pid);

	/* go to last entry */
	n = vfs_node_openDir(proc,true);
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0) {
			vfs_node_closeDir(proc,true);
			res = ERR_FILE_EXISTS;
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
	vfs_real_removeProc(pid);
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

	/* build name */
	name = (char*)cache_alloc(12);
	if(name == NULL)
		return;
	itoa(name,12,tid);

	/* search for thread-node and remove it */
	dir = vfs_node_get(t->proc->threadDir);
	n = vfs_node_openDir(dir,true);
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
	sVFSNode *drv = vfs_node_openDir(devNode,true);
	vid_printf("Messages:\n");
	while(drv != NULL) {
		if(IS_DRIVER(drv->mode))
			vfs_server_print(drv);
		drv = drv->next;
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
			if(e->flags & VFS_DRIVER)
				vid_printf("DRIVER ");
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
				if(IS_CHANNEL(n->mode))
					vid_printf("\t\tDriver-Usage: %s @ %s\n",n->name,n->parent->name);
				else
					vid_printf("\t\tFile: '%s'\n",vfs_node_getPath(vfs_node_getNo(n)));
			}
		}
	}
}
