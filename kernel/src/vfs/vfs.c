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
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/real.h>
#include <vfs/info.h>
#include <vfs/request.h>
#include <vfs/driver.h>
#include <vfs/rw.h>
#include <vfs/listeners.h>
#include <task/proc.h>
#include <task/sched.h>
#include <mem/paging.h>
#include <mem/kheap.h>
#include <util.h>
#include <video.h>
#include <kevent.h>
#include <string.h>
#include <sllist.h>
#include <assert.h>
#include <errors.h>
#include <messages.h>

/* max number of open files */
#define FILE_COUNT					(PROC_COUNT * 16)
/* the processes node */
#define PROCESSES()					(nodes + 7)
/* the services node */
#define SERVICES()					(nodes + 10)
/* the drivers-node */
#define DRIVERS()					(nodes + 19)

/* an entry in the global file table */
typedef struct {
	/* read OR write; flags = 0 => entry unused */
	u16 flags;
	/* the owner of this file; sharing of a file is not possible for different threads
	 * (an exception is the inheritance to a child-thread) */
	tTid owner;
	/* number of references */
	u16 refCount;
	/* current position in file */
	u32 position;
	/* node-number */
	tInodeNo nodeNo;
	/* the device-number */
	tDevNo devNo;
} sGFTEntry;

/**
 * Searches for a free file for the given flags and node-number
 *
 * @param tid the thread to use
 * @param flags the flags (read, write)
 * @param nodeNo the node-number to open
 * @param devNo the device-number
 * @return the file-number on success or the negative error-code
 */
static tFileNo vfs_getFreeFile(tTid tid,u16 flags,tInodeNo nodeNo,tDevNo devNo);

/* global file table */
static sGFTEntry globalFileTable[FILE_COUNT];

void vfs_init(void) {
	sVFSNode *root,*sys;
	vfsn_init();

	/*
	 *  /
	 *   system
	 *     |-pipe
	 *     |-processes
	 *     |-devices
	 *     |-bin
	 *   services
	 *   drivers
	 */
	root = vfsn_createDir(NULL,(char*)"");
	sys = vfsn_createDir(root,(char*)"system");
	vfsn_createPipeCon(sys,(char*)"pipe");
	vfsn_createDir(sys,(char*)"processes");
	vfsn_createDir(root,(char*)"services");
	vfsn_createDir(sys,(char*)"devices");
	vfsn_createDir(sys,(char*)"bin");
	vfsn_createDir(root,(char*)"drivers");
}

s32 vfs_hasAccess(tTid tid,tInodeNo nodeNo,u16 flags) {
	sVFSNode *n = nodes + nodeNo;
	/* kernel is allmighty :P */
	if(tid == KERNEL_TID)
		return 0;

	if(n->owner == tid) {
		if((flags & VFS_READ) && !(n->mode & MODE_OWNER_READ))
			return ERR_NO_READ_PERM;
		if((flags & VFS_WRITE) && !(n->mode & MODE_OWNER_WRITE))
			return ERR_NO_WRITE_PERM;
	}
	else {
		if((flags & VFS_READ) && !(n->mode & MODE_OTHER_READ))
			return ERR_NO_READ_PERM;
		if((flags & VFS_WRITE) && !(n->mode & MODE_OTHER_WRITE))
			return ERR_NO_WRITE_PERM;
	}
	return 0;
}

tFileNo vfs_inheritFileNo(tTid tid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfsn_getNode(e->nodeNo);
	vassert(e->flags != 0,"Invalid file %d",file);
	/* we can't share service-usages since each thread has his own node */
	if(e->devNo == VFS_DEV_NO && (n->mode & MODE_TYPE_SERVUSE)) {
		sVFSNode *child;
		tInodeNo nodeNo;
		tFileNo newFile;
		s32 err = vfsn_createServiceUse(tid,n->parent,&child);
		if(err < 0)
			return -1;

		nodeNo = NADDR_TO_VNNO(child);
		newFile = vfs_openFile(tid,e->flags & (VFS_READ | VFS_WRITE),nodeNo,VFS_DEV_NO);
		if(newFile < 0) {
			vfsn_removeNode(child);
			return -1;
		}
		return newFile;
	}
	/* if a pipe is inherited we need a new file for it (position should be different )*/
	else if(e->devNo == VFS_DEV_NO && (n->mode & MODE_TYPE_PIPE)) {
		tFileNo newFile;
		/* we'll get a new file since the tid is different */
		newFile = vfs_openFile(tid,e->flags & (VFS_READ | VFS_WRITE),e->nodeNo,e->devNo);
		if(newFile < 0)
			return -1;
		return newFile;
	}
	else {
		/* just increase references */
		e->refCount++;
		return file;
	}
}

s32 vfs_incRefs(tFileNo file) {
	sGFTEntry *e;
	/* invalid file-number? */
	if(file < 0 || file >= FILE_COUNT)
		return ERR_INVALID_FILE;

	e = globalFileTable + file;
	if(e->flags == 0)
		return ERR_INVALID_FILE;

	e->refCount++;
	return 0;
}

s32 vfs_getOwner(tFileNo file) {
	sGFTEntry *e;

	/* invalid file-number? */
	if(file < 0 || file >= FILE_COUNT)
		return ERR_INVALID_FILE;

	/* not in use? */
	e = globalFileTable + file;
	if(e->flags == 0)
		return ERR_INVALID_FILE;
	return e->owner;
}

s32 vfs_getFileId(tFileNo file,tInodeNo *ino,tDevNo *dev) {
	sGFTEntry *e;

	/* invalid file-number? */
	if(file < 0 || file >= FILE_COUNT)
		return ERR_INVALID_FILE;

	/* not in use? */
	e = globalFileTable + file;
	if(e->flags == 0)
		return ERR_INVALID_FILE;

	*ino = e->nodeNo;
	*dev = e->devNo;
	return 0;
}

tFileNo vfs_openFile(tTid tid,u16 flags,tInodeNo nodeNo,tDevNo devNo) {
	sGFTEntry *e;
	sVFSNode *n = NULL;
	tFileNo f;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_CREATED;

	/* determine free file */
	f = vfs_getFreeFile(tid,flags,nodeNo,devNo);
	if(f < 0)
		return f;

	if(devNo == VFS_DEV_NO) {
		s32 err;
		n = nodes + nodeNo;
		if((err = vfs_hasAccess(tid,nodeNo,flags)) < 0)
			return err;
	}

	/* unused file? */
	e = globalFileTable + f;
	if(e->flags == 0) {
		/* count references of virtual nodes */
		if(devNo == VFS_DEV_NO)
			nodes[nodeNo].refCount++;
		e->owner = tid;
		e->flags = flags;
		e->refCount = 1;
		e->position = 0;
		e->devNo = devNo;
		e->nodeNo = nodeNo;
	}
	else
		e->refCount++;

	return f;
}

static tFileNo vfs_getFreeFile(tTid tid,u16 flags,tInodeNo nodeNo,tDevNo devNo) {
	tFileNo i;
	tFileNo freeSlot = ERR_NO_FREE_FILE;
	u16 rwFlags = flags & (VFS_READ | VFS_WRITE);
	bool isServUse = false;
	sGFTEntry *e = &globalFileTable[0];

	/* ensure that we don't increment usages of an unused slot */
	vassert(flags & (VFS_READ | VFS_WRITE),"flags empty");
	vassert(!(flags & ~(VFS_READ | VFS_WRITE | VFS_CREATED)),"flags contains invalid bits");

	if(devNo == VFS_DEV_NO) {
		sVFSNode *n = vfsn_getNode(nodeNo);
		vassert(nodeNo < NODE_COUNT,"nodeNo invalid");
		/* we can add pipes here, too, since every open() to a pipe will get a new node anyway */
		isServUse = (n->mode & (MODE_TYPE_SERVUSE | MODE_TYPE_PIPE)) ? true : false;
	}

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0) {
			/* we don't want to share files with different threads */
			/* this is allowed only if we create a child-threads. he will inherit the files.
			 * in this case we trust the threads that they know what they do :) */
			if(e->devNo == devNo && e->nodeNo == nodeNo && e->owner == tid) {
				/* service-usages may use a file twice for reading and writing because we
				 * will prevent trouble anyway */
				if(isServUse && (e->flags & (VFS_READ | VFS_WRITE)) == rwFlags)
					return i;

				/* someone does already write to this file? so it's not really good
				 * to use it atm, right? */
				/* TODO this means that a different thread could open the same file for reading
				 * or writing while another one is writing!? */
				if(!isServUse && e->flags & VFS_WRITE)
					return ERR_FILE_IN_USE;

				/* if the flags are different we need a different slot */
				if((e->flags & (VFS_READ | VFS_WRITE)) == rwFlags)
					return i;
			}
		}
		/* remember free slot */
		else if(freeSlot == ERR_NO_FREE_FILE) {
			freeSlot = i;
			/* just for performance: if we've found an unused file and want to use a service,
			 * use this slot because it doesn't really matter wether we use a new file or an
			 * existing one (if there even is any) */
			/* note: we can share a file for writing in this case! */
			if(isServUse)
				break;
		}

		e++;
	}

	return freeSlot;
}

u32 vfs_tell(tTid tid,tFileNo file) {
	UNUSED(tid);
	sGFTEntry *e = globalFileTable + file;
	vassert(e->flags != 0,"Invalid file %d",file);
	return e->position;
}

bool vfs_eof(tTid tid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	bool eof = true;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = nodes + e->nodeNo;

		if(n->mode & MODE_TYPE_SERVUSE) {
			if(IS_DRIVER(n->parent->mode))
				eof = n->parent->data.service.isEmpty;
			else if(n->parent->owner == tid)
				eof = sll_length(n->data.servuse.sendList) == 0;
			else
				eof = sll_length(n->data.servuse.recvList) == 0;
		}
		/* we've read all from a pipe if there is one zero-length-entry left */
		else if(n->mode & MODE_TYPE_PIPE)
			eof = sll_length(n->data.pipe.list) == 1 && n->data.pipe.total == 0;
		else
			eof = e->position >= n->data.def.pos;
	}
	else {
		sFileInfo info;
		vfsr_istat(tid,e->nodeNo,e->devNo,&info);
		eof = (s32)e->position >= info.size;
	}

	return eof;
}

s32 vfs_seek(tTid tid,tFileNo file,s32 offset,u32 whence) {
	sGFTEntry *e = globalFileTable + file;
	s32 newPos;
	UNUSED(tid);
	vassert(e->flags != 0,"Invalid file %d",file);

	switch(whence) {
		case SEEK_SET:
			newPos = offset;
			break;
		case SEEK_CUR:
			newPos = (s32)e->position + offset;
			break;
		case SEEK_END:
		default:
			newPos = 0;
			break;
	}

	if(newPos < 0)
		return ERR_INVALID_ARGS;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = nodes + e->nodeNo;
		/* no seek for pipes */
		if(n->mode & MODE_TYPE_PIPE)
			return ERR_PIPE_SEEK;

		if(n->mode & MODE_TYPE_SERVUSE) {
			if(IS_DRIVER(n->parent->mode)) {
				/* not supported for drivers */
				if(whence == SEEK_END)
					return ERR_INVALID_ARGS;
				e->position = newPos;
			}
			else {
				sSLList *list;
				/* just SEEK_CUR and SEEK_END is supported */
				if(whence != SEEK_CUR && whence != SEEK_END)
					return ERR_SERVUSE_SEEK;

				/* services read from the send-list */
				if(n->parent->owner == tid)
					list = n->data.servuse.sendList;
				/* other processes read to the receive-list*/
				else
					list = n->data.servuse.recvList;

				/* skip the next <offset> messages */
				if(list) {
					if(whence == SEEK_END)
						offset = sll_length(list);
					while(offset-- > 0) {
						if(!sll_removeFirst(list,NULL))
							break;
					}
				}
			}
		}
		else {
			if(whence == SEEK_END)
				e->position = n->data.def.pos;
			else {
				/* we can't validate the position here because the content of virtual files may be
				 * generated on demand */
				e->position = newPos;
			}
		}
	}
	else {
		if(whence == SEEK_END) {
			sFileInfo info;
			s32 res = vfsr_istat(tid,e->nodeNo,e->devNo,&info);
			if(res < 0)
				return res;
			e->position = info.size;
		}
		else {
			/* since the fs-service validates the position anyway we can simply set it */
			e->position = newPos;
		}
	}

	return e->position;
}

s32 vfs_readFile(tTid tid,tFileNo file,u8 *buffer,u32 count) {
	s32 err,readBytes;
	sGFTEntry *e = globalFileTable + file;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(!(e->flags & VFS_READ))
		return ERR_NO_READ_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = nodes + e->nodeNo;

		if((err = vfs_hasAccess(tid,e->nodeNo,VFS_READ)) < 0)
			return err;

		/* node not present anymore or no read-handler? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
		if(n->readHandler == NULL)
			return ERR_NO_READ_PERM;

		/* use the read-handler */
		readBytes = n->readHandler(tid,file,n,buffer,e->position,count);
		if(readBytes > 0)
			e->position += readBytes;
	}
	else {
		/* query the fs-service to read from the inode */
		readBytes = vfsr_readFile(tid,file,e->nodeNo,e->devNo,buffer,e->position,count);
		if(readBytes > 0)
			e->position += readBytes;
	}

	return readBytes;
}

s32 vfs_writeFile(tTid tid,tFileNo file,const u8 *buffer,u32 count) {
	s32 err,writtenBytes;
	sGFTEntry *e = globalFileTable + file;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(!(e->flags & VFS_WRITE))
		return ERR_NO_WRITE_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = nodes + e->nodeNo;

		if((err = vfs_hasAccess(tid,e->nodeNo,VFS_WRITE)) < 0)
			return err;

		/* node not present anymore or no write-handler? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
		if(n->writeHandler == NULL)
			return ERR_NO_WRITE_PERM;

		/* write to the node */
		writtenBytes = n->writeHandler(tid,file,n,buffer,e->position,count);
		if(writtenBytes > 0) {
			e->flags |= VFS_MODIFIED;
			e->position += writtenBytes;
		}
	}
	else {
		/* query the fs-service to write to the inode */
		writtenBytes = vfsr_writeFile(tid,file,e->nodeNo,e->devNo,buffer,e->position,count);
		if(writtenBytes > 0)
			e->position += writtenBytes;
	}

	return writtenBytes;
}

s32 vfs_ioctl(tTid tid,tFileNo file,u32 cmd,u8 *data,u32 size) {
	s32 err;
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO)
		return ERR_INVALID_FILE;

	n = nodes + e->nodeNo;

	/* TODO keep this? */
	if((err = vfs_hasAccess(tid,e->nodeNo,VFS_WRITE)) < 0)
		return err;

	/* node not present anymore? */
	if(n->name == NULL || !(n->mode & MODE_TYPE_SERVUSE) || !IS_DRIVER(n->parent->mode))
		return ERR_INVALID_FILE;

	return vfsdrv_ioctl(tid,file,n,cmd,data,size);
}

s32 vfs_sendMsg(tTid tid,tFileNo file,tMsgId id,const u8 *data,u32 size) {
	s32 err;
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO)
		return ERR_INVALID_FILE;

	n = nodes + e->nodeNo;

	if((err = vfs_hasAccess(tid,e->nodeNo,VFS_WRITE)) < 0)
		return err;

	/* node not present anymore? */
	if(n->name == NULL)
		return ERR_INVALID_FILE;

	/* send the message */
	return vfsrw_writeServUse(tid,file,n,id,data,size);
}

s32 vfs_receiveMsg(tTid tid,tFileNo file,tMsgId *id,u8 *data,u32 size) {
	s32 err;
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO)
		return ERR_INVALID_FILE;

	n = nodes + e->nodeNo;

	if((err = vfs_hasAccess(tid,e->nodeNo,VFS_READ)) < 0)
		return err;

	/* node not present anymore? */
	if(n->name == NULL)
		return ERR_INVALID_FILE;

	/* send the message */
	return vfsrw_readServUse(tid,file,n,id,data,size);
}

void vfs_closeFile(tTid tid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	vassert(e->flags != 0,"Invalid file %d",file);

	/* decrement references */
	if(--(e->refCount) == 0) {
		if(e->devNo == VFS_DEV_NO) {
			sVFSNode *n = nodes + e->nodeNo;
			if(n->name != NULL) {
				/* notify listeners about creation/modification of files */
				/* TODO what about links ? */
				if(MODE_IS_FILE(n->mode)) {
					if(e->flags & VFS_CREATED)
						vfsl_notify(n,VFS_EV_CREATE);
					else if(e->flags & VFS_MODIFIED)
						vfsl_notify(n,VFS_EV_MODIFY);
				}

				/* last usage? */
				if(--(n->refCount) == 0) {
					/* notify the driver, if it is one */
					if((n->mode & MODE_TYPE_SERVUSE) && IS_DRIVER(n->parent->mode))
						vfsdrv_close(tid,file,n);

					/* remove pipe-nodes if there are no references anymore */
					if(n->mode & MODE_TYPE_PIPE)
						vfsn_removeNode(n);
					/* if there are message for the service we don't want to throw them away */
					/* if there are any in the receivelist (and no references of the node) we
					 * can throw them away because no one will read them anymore
					 * (it means that the client has already closed the file) */
					/* note also that we can assume that the service is still running since we
					 * would have deleted the whole service-node otherwise */
					else if((n->mode & MODE_TYPE_SERVUSE) &&
							sll_length(n->data.servuse.sendList) == 0)
						vfsn_removeNode(n);
				}
				/* if we're the owner of the pipe, append an "EOF-message" */
				/* otherwise we have a problem when the fd is inherited to multiple processes.
				 * in this case all these processes would write an EOF (and of course not necessarily
				 * at the right place) */
				else if(n->owner == tid && (n->mode & MODE_TYPE_PIPE))
					vfsrw_writePipe(tid,file,n,NULL,e->position,0);
			}
		}
		else
			vfsr_closeFile(tid,file,e->nodeNo,e->devNo);

		/* mark unused */
		e->flags = 0;
	}
}

s32 vfs_link(tTid tid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN];
	char *name,*namecpy,backup;
	u32 len;
	tInodeNo oldIno,newIno;
	sVFSNode *dir,*target;
	s32 oldRes,newRes;
	/* first check wether it is a realpath */
	oldRes = vfsn_resolvePath(oldPath,&oldIno,NULL,VFS_READ);
	newRes = vfsn_resolvePath(newPath,&newIno,NULL,VFS_WRITE);
	if(oldRes == ERR_REAL_PATH) {
		if(newRes != ERR_REAL_PATH)
			return ERR_LINK_DEVICE;
		return vfsr_link(tid,oldPath,newPath);
	}
	if(oldRes < 0)
		return oldRes;
	if(newRes >= 0)
		return ERR_FILE_EXISTS;

	/* TODO check access-rights */

	/* copy path because we have to change it */
	len = strlen(newPath);
	if(len >= MAX_PATH_LEN)
		return ERR_INVALID_PATH;
	strcpy(newPathCpy,newPath);
	/* check wether the directory exists */
	name = vfsn_basename((char*)newPathCpy,&len);
	backup = *name;
	vfsn_dirname((char*)newPathCpy,len);
	newRes = vfsn_resolvePath(newPathCpy,&newIno,NULL,VFS_WRITE);
	if(newRes < 0)
		return ERR_PATH_NOT_FOUND;

	/* links to directories not allowed */
	target = vfsn_getNode(oldIno);
	if(MODE_IS_DIR(target->mode))
		return ERR_IS_DIR;

	/* make copy of name */
	*name = backup;
	len = strlen(name);
	namecpy = kheap_alloc(len + 1);
	if(namecpy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(namecpy,name);
	/* now create link */
	dir = vfsn_getNode(newIno);
	/* file exists? */
	if(vfsn_findInDir(dir,namecpy,len) != NULL) {
		kheap_free(namecpy);
		return ERR_FILE_EXISTS;
	}
	if(vfsn_createLink(dir,namecpy,target) == NULL) {
		kheap_free(namecpy);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 vfs_unlink(tTid tid,const char *path) {
	s32 res;
	tInodeNo ino;
	sVFSNode *n;
	res = vfsn_resolvePath(path,&ino,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == ERR_REAL_PATH)
		return vfsr_unlink(tid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;
	/* TODO check access-rights */
	n = vfsn_getNode(ino);
	if(!MODE_IS_FILE(n->mode) && !MODE_IS_LINK(n->mode))
		return ERR_NO_FILE_OR_LINK;
	/* notify listeners about deletion */
	if(MODE_IS_FILE(n->mode))
		vfsl_notify(n,VFS_EV_DELETE);
	vfsn_removeNode(n);
	return 0;
}

s32 vfs_mkdir(tTid tid,const char *path) {
	char pathCpy[MAX_PATH_LEN];
	char *name,*namecpy;
	char backup;
	s32 res;
	u32 len = strlen(path);
	tInodeNo inodeNo;
	sVFSNode *node,*child;

	/* copy path because we'll change it */
	if(len >= MAX_PATH_LEN)
		return ERR_INVALID_PATH;
	strcpy(pathCpy,path);

	/* extract name and directory */
	name = vfsn_basename(pathCpy,&len);
	backup = *name;
	vfsn_dirname(pathCpy,len);

	/* get the parent-directory */
	res = vfsn_resolvePath(pathCpy,&inodeNo,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == ERR_REAL_PATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		/* let fs handle the request */
		return vfsr_mkdir(tid,path);
	}
	if(res < 0)
		return res;

	/* alloc space for name and copy it over */
	*name = backup;
	len = strlen(name);
	namecpy = kheap_alloc(len + 1);
	if(namecpy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(namecpy,name);
	/* create dir */
	node = vfsn_getNode(inodeNo);
	if(vfsn_findInDir(node,namecpy,len) != NULL) {
		kheap_free(namecpy);
		return ERR_FILE_EXISTS;
	}
	/* TODO check access-rights */
	child = vfsn_createDir(node,namecpy);
	if(child == NULL) {
		kheap_free(namecpy);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 vfs_rmdir(tTid tid,const char *path) {
	s32 res;
	sVFSNode *node;
	tInodeNo inodeNo;
	res = vfsn_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == ERR_REAL_PATH)
		return vfsr_rmdir(tid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;

	/* TODO check access-rights */
	node = vfsn_getNode(inodeNo);
	if(!MODE_IS_DIR(node->mode))
		return ERR_NO_DIRECTORY;
	vfsn_removeNode(node);
	return 0;
}

tServ vfs_createService(tTid tid,const char *name,u32 type) {
	sVFSNode *serv;
	sVFSNode *n;
	u32 len;
	char *hname;

	/* determine position in VFS */
	if(IS_DRIVER(type))
		serv = DRIVERS();
	else
		serv = SERVICES();
	n = serv->firstChild;

	vassert(name != NULL,"name == NULL");

	/* we don't want to have exotic service-names */
	if((len = strlen(name)) == 0 || !isalnumstr(name))
		return ERR_INV_SERVICE_NAME;

	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			return ERR_SERVICE_EXISTS;
		n = n->next;
	}

	/* copy name to kernel-heap */
	hname = (char*)kheap_alloc(len + 1);
	if(hname == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strncpy(hname,name,len);
	hname[len] = '\0';

	/* create node */
	n = vfsn_createServiceNode(tid,serv,hname,type);
	if(n != NULL)
		return NADDR_TO_VNNO(n);

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_setDataReadable(tTid tid,tInodeNo nodeNo,bool readable) {
	sVFSNode *n = nodes + nodeNo;
	if(n->name == NULL || !IS_DRIVER(n->mode))
		return ERR_INVALID_SERVID;
	if(n->owner != tid)
		return ERR_NOT_OWN_SERVICE;

	n->data.service.isEmpty = !readable;
	if(readable)
		thread_wakeupAll(EV_DATA_READABLE | EV_RECEIVED_MSG);
	return 0;
}

bool vfs_msgAvailableFor(tTid tid,u8 events) {
	sVFSNode *n;
	sThread *t = thread_getById(tid);
	bool isService = false;
	bool isClient = false;
	tFD i;

	/* at first we check wether the process is a service */
	if(events & EV_CLIENT) {
		sVFSNode *rn[2] = {SERVICES(),DRIVERS()};
		for(i = 0; i < 2; i++) {
			n = NODE_FIRST_CHILD(rn[i]);
			while(n != NULL) {
				if(n->owner == tid) {
					tInodeNo nodeNo = NADDR_TO_VNNO(n);
					tInodeNo client = vfs_getClient(tid,&nodeNo,1);
					isService = true;
					if(vfsn_isValidNodeNo(client))
						return true;
				}
				n = n->next;
			}
		}
	}

	/* now search through the file-descriptors if there is any message */
	if(events & EV_RECEIVED_MSG) {
		for(i = 0; i < MAX_FD_COUNT; i++) {
			if(t->fileDescs[i] != -1) {
				sGFTEntry *e = globalFileTable + t->fileDescs[i];
				if(e->devNo == VFS_DEV_NO) {
					n = vfsn_getNode(e->nodeNo);
					/* service-usage and a message in the receive-list? */
					/* we don't want to check that if it is our own service. because it makes no
					 * sense to read from ourself ;) */
					if((n->mode & MODE_TYPE_SERVUSE) && n->parent->owner != tid) {
						isClient = true;
						if(n->data.servuse.recvList != NULL && sll_length(n->data.servuse.recvList) > 0)
							return true;
					}
				}
			}
		}
	}

	/* if we are no client and no service we'll never receive a message */
	/*if(isClient || isService)*/
		return false;
	/*return true;*/
}

s32 vfs_getClient(tTid tid,tInodeNo *vfsNodes,u32 count) {
	sVFSNode *n,*node = NULL,*last,*match = NULL;
	u32 i;
	bool skipped;
	/* this is a bit more complicated because we want to do it in a fair way. that means every
	 * process that requests something should be served at some time. therefore we store the last
	 * served client and continue from the next one. */
retry:
	skipped = false;
	for(i = 0; i < count; i++) {
		if(!vfsn_isValidNodeNo(vfsNodes[i]))
			return ERR_INVALID_SERVID;

		node = nodes + vfsNodes[i];
		if(node->owner != tid || !(node->mode & MODE_TYPE_SERVICE))
			return ERR_NOT_OWN_SERVICE;

		/* search for a slot that needs work */
		last = node->data.service.lastClient;
		if(last == NULL)
			n = NODE_FIRST_CHILD(node);
		else if(last->next == NULL) {
			/* if we have checked all clients in this service, give the other services
			 * a chance (if there are any others) */
			node->data.service.lastClient = NULL;
			skipped = true;
			continue;
		}
		else
			n = last->next;
searchBegin:
		while(n != NULL && n != last) {
			/* data available? */
			if(sll_length(n->data.servuse.sendList) > 0) {
				node->data.service.lastClient = n;
				return NADDR_TO_VNNO(n);
			}
			n = n->next;
		}
		/* if we have looked through all nodes and the last one has a message again, we have to
		 * store it because we'll not check it again */
		if(last && n == last && sll_length(n->data.servuse.sendList) > 0)
			match = n;
		else if(node->data.service.lastClient) {
			n = NODE_FIRST_CHILD(node);
			node->data.service.lastClient = NULL;
			goto searchBegin;
		}
	}
	/* if we have a match, use this one */
	if(match) {
		node->data.service.lastClient = match;
		return NADDR_TO_VNNO(match);
	}
	/* if not and we've skipped a client, try another time */
	if(skipped)
		goto retry;
	return ERR_NO_CLIENT_WAITING;
}

tFileNo vfs_openClientThread(tTid tid,tInodeNo nodeNo,tTid clientId) {
	sVFSNode *n,*node;
	/* check if the node is valid */
	if(!vfsn_isValidNodeNo(nodeNo))
		return ERR_INVALID_SERVID;
	node = nodes + nodeNo;
	if(node->owner != tid || !(node->mode & MODE_TYPE_SERVICE))
		return ERR_NOT_OWN_SERVICE;

	/* search for a slot that needs work */
	n = NODE_FIRST_CHILD(node);
	while(n != NULL) {
		if(n->owner == clientId)
			break;
		n = n->next;
	}

	/* not found? */
	if(n == NULL)
		return ERR_PATH_NOT_FOUND;

	/* open file */
	return vfs_openFile(tid,VFS_READ | VFS_WRITE,NADDR_TO_VNNO(n),VFS_DEV_NO);
}

tFileNo vfs_openClient(tTid tid,tInodeNo *vfsNodes,u32 count,tInodeNo *servNode) {
	sVFSNode *n;
	tInodeNo client = vfs_getClient(tid,vfsNodes,count);
	/* error? */
	if(!vfsn_isValidNodeNo(client))
		return client;

	/* open a file for it so that the service can read and write with it */
	n = vfsn_getNode(client);
	*servNode = NADDR_TO_VNNO(n->parent);
	return vfs_openFile(tid,VFS_READ | VFS_WRITE,client,VFS_DEV_NO);
}

s32 vfs_removeService(tTid tid,tInodeNo nodeNo) {
	sVFSNode *n = nodes + nodeNo;

	vassert(vfsn_isValidNodeNo(nodeNo),"Invalid node number %d",nodeNo);

	if(n->owner != tid || !(n->mode & MODE_TYPE_SERVICE))
		return ERR_NOT_OWN_SERVICE;

	/* remove service-node including all service-usages */
	vfsn_removeNode(n);
	return 0;
}

sVFSNode *vfs_createProcess(tPid pid,fRead handler) {
	char *name;
	sVFSNode *proc = PROCESSES();
	sVFSNode *n = proc->firstChild;
	sVFSNode *dir,*tdir;

	/* build name */
	name = (char*)kheap_alloc(sizeof(char) * 12);
	if(name == NULL)
		return NULL;

	itoa(name,pid);

	/* go to last entry */
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			goto errorName;
		n = n->next;
	}

	/* create dir */
	dir = vfsn_createDir(proc,name);
	if(dir == NULL)
		goto errorName;

	/* invalidate cache */
	if(proc->data.def.cache != NULL) {
		kheap_free(proc->data.def.cache);
		proc->data.def.cache = NULL;
	}

	/* create process-info-node */
	n = vfsn_createFile(KERNEL_TID,dir,(char*)"info",handler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create virt-mem-info-node */
	n = vfsn_createFile(KERNEL_TID,dir,(char*)"virtmem",vfsinfo_virtMemReadHandler,NULL);
	if(n == NULL)
		goto errorDir;

	/* create threads-dir */
	tdir = vfsn_createDir(dir,(char*)"threads");
	if(tdir == NULL)
		goto errorDir;

	return tdir;

errorDir:
	vfsn_removeNode(dir);
errorName:
	kheap_free(name);
	return NULL;
}

void vfs_removeProcess(tPid pid) {
	sVFSNode *proc = PROCESSES();
	sProc *p = proc_getByPid(pid);
	char name[12];
	itoa(name,pid);

	/* remove from /system/processes */
	vfsn_removeNode(p->threadDir->parent);

	/* invalidate cache */
	if(proc->data.def.cache != NULL) {
		kheap_free(proc->data.def.cache);
		proc->data.def.cache = NULL;
	}
}

bool vfs_createThread(tTid tid,fRead handler) {
	char *name;
	sVFSNode *n;
	sThread *t = thread_getById(tid);

	/* build name */
	name = (char*)kheap_alloc(sizeof(char) * 12);
	if(name == NULL)
		return false;
	itoa(name,tid);

	/* create thread-node */
	n = vfsn_createFile(tid,t->proc->threadDir,name,handler,NULL);
	if(n == NULL) {
		kheap_free(name);
		return false;
	}

	return true;
}

void vfs_removeThread(tTid tid) {
	sThread *t = thread_getById(tid);
	sVFSNode *rn[2] = {SERVICES(),DRIVERS()};
	sVFSNode *n,*tn;
	u32 i;
	char *name;

	/* check if the thread is a service */
	for(i = 0; i < 2; i++) {
		n = NODE_FIRST_CHILD(rn[i]->firstChild);
		while(n != NULL) {
			if((n->mode & MODE_TYPE_SERVICE) && n->owner == tid) {
				tn = n->next;
				vfs_removeService(tid,NADDR_TO_VNNO(n));
				n = tn;
			}
			else
				n = n->next;
		}
	}

	/* build name */
	name = (char*)kheap_alloc(sizeof(char) * 12);
	if(name == NULL)
		return;
	itoa(name,tid);

	/* search for thread-node and remove it */
	n = NODE_FIRST_CHILD(t->proc->threadDir);
	while(n != NULL) {
		if(strcmp(n->name,name) == 0) {
			vfsn_removeNode(n);
			break;
		}
		n = n->next;
	}

	kheap_free(name);
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

u32 vfs_dbg_getGFTEntryCount(void) {
	u32 i,count = 0;
	for(i = 0; i < FILE_COUNT; i++) {
		if(globalFileTable[i].flags != 0)
			count++;
	}
	return count;
}

void vfs_dbg_printGFT(void) {
	tFileNo i;
	sGFTEntry *e = globalFileTable;
	vid_printf("Global File Table:\n");
	for(i = 0; i < FILE_COUNT; i++) {
		if(e->flags != 0) {
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tread: %d\n",(e->flags & VFS_READ) ? true : false);
			vid_printf("\t\twrite: %d\n",(e->flags & VFS_WRITE) ? true : false);
			vid_printf("\t\tnodeNo: %d\n",e->nodeNo);
			vid_printf("\t\tdevNo: %d\n",e->devNo);
			vid_printf("\t\tpos: %d\n",e->position);
			vid_printf("\t\trefCount: %d\n",e->refCount);
			vid_printf("\t\towner: %d\n",e->owner);
			if(e->devNo == VFS_DEV_NO) {
				sVFSNode *n = vfsn_getNode(e->nodeNo);
				if(n->mode & MODE_TYPE_SERVUSE)
					vid_printf("\t\tService-Usage of %s @ %s\n",n->name,n->parent->name);
			}
		}
		e++;
	}
}

#endif
