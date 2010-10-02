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
#include <sys/task/event.h>
#include <sys/mem/paging.h>
#include <sys/mem/kheap.h>
#include <sys/mem/dynarray.h>
#include <sys/util.h>
#include <sys/video.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

#define FILE_COUNT					((s32)(gftArray.objCount))

/* an entry in the global file table */
typedef struct {
	/* read OR write; flags = 0 => entry unused */
	u16 flags;
	/* the owner of this file */
	tPid owner;
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
 * Checks whether the process with given pid has the permission to do the given stuff with <nodeNo>.
 *
 * @param pid the process-id
 * @param nodeNo the node-number
 * @param flags specifies what you want to do (VFS_READ | VFS_WRITE)
 * @return 0 if the process has permission or the error-code
 */
static s32 vfs_hasAccess(tPid pid,tInodeNo nodeNo,u16 flags);
/**
 * Searches for a free file for the given flags and node-number
 *
 * @param pid the process to use
 * @param flags the flags (read, write)
 * @param nodeNo the node-number to open
 * @param devNo the device-number
 * @return the file-number on success or the negative error-code
 */
static tFileNo vfs_getFreeFile(tPid pid,u16 flags,tInodeNo nodeNo,tDevNo devNo);

/* global file table (expands dynamically) */
static sDynArray gftArray;
static sGFTEntry *globalFileTable = (sGFTEntry*)GFT_AREA;
static sVFSNode *procsNode;
static sVFSNode *devNode;

void vfs_init(void) {
	sVFSNode *root,*sys;
	dyna_init(&gftArray,sizeof(sGFTEntry),GFT_AREA,GFT_AREA_SIZE);
	vfs_node_init();

	/*
	 *  /
	 *   system
	 *     |-pipe
	 *     |-processes
	 *     |-devices
	 *   dev
	 */
	root = vfs_dir_create(KERNEL_PID,NULL,(char*)"");
	sys = vfs_dir_create(KERNEL_PID,root,(char*)"system");
	vfs_dir_create(KERNEL_PID,sys,(char*)"pipe");
	procsNode = vfs_dir_create(KERNEL_PID,sys,(char*)"processes");
	vfs_dir_create(KERNEL_PID,sys,(char*)"devices");
	devNode = vfs_dir_create(KERNEL_PID,root,(char*)"dev");
}

static s32 vfs_hasAccess(tPid pid,tInodeNo nodeNo,u16 flags) {
	sVFSNode *n = vfs_node_get(nodeNo);
	if(n->name == NULL)
		return ERR_INVALID_FILE;
	/* kernel is allmighty :P */
	if(pid == KERNEL_PID)
		return 0;

	if(n->owner == pid) {
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

s32 vfs_incRefs(tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	if(file < 0 || file >= FILE_COUNT || e->flags == 0)
		return ERR_INVALID_FILE;
	e->refCount++;
	return 0;
}

sVFSNode *vfs_getVNode(tFileNo file) {
	sVFSNode *n;
	sGFTEntry *e = globalFileTable + file;
	if(file < 0 || file >= FILE_COUNT || e->flags == 0)
		return NULL;
	if(e->devNo != VFS_DEV_NO)
		return NULL;
	n = vfs_node_get(e->nodeNo);
	if(n->name == NULL)
		return NULL;
	return n;
}

s32 vfs_getFileId(tFileNo file,tInodeNo *ino,tDevNo *dev) {
	sGFTEntry *e = globalFileTable + file;
	if(file < 0 || file >= FILE_COUNT || e->flags == 0)
		return ERR_INVALID_FILE;

	*ino = e->nodeNo;
	*dev = e->devNo;
	return 0;
}

s32 vfs_fcntl(tPid pid,tFileNo file,u32 cmd,s32 arg) {
	UNUSED(pid);
	sGFTEntry *e = globalFileTable + file;
	assert(file >= 0 && file < FILE_COUNT);
	switch(cmd) {
		case F_GETFL:
			return e->flags;
		case F_SETFL:
			e->flags &= VFS_READ | VFS_WRITE | VFS_CREATE;
			e->flags |= arg & VFS_NOBLOCK;
			return 0;
		case F_SETDATA: {
			sVFSNode *n = vfs_node_get(e->nodeNo);
			if(e->devNo != VFS_DEV_NO || n->name == NULL || !IS_DRIVER(n->mode))
				return ERR_INVALID_ARGS;
			return vfs_server_setReadable(n,(bool)arg);
		}
	}
	return ERR_INVALID_ARGS;
}

bool vfs_shouldBlock(tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	assert(file >= 0 && file < FILE_COUNT);
	return !(e->flags & VFS_NOBLOCK);
}

tFileNo vfs_openPath(tPid pid,u16 flags,const char *path) {
	tInodeNo nodeNo;
	tFileNo file;
	bool created;
	sVFSNode *node = NULL;

	/* resolve path */
	s32 err = vfs_node_resolvePath(path,&nodeNo,&created,flags);
	if(err == ERR_REAL_PATH) {
		/* unfortunatly we have to check for the process-ids of ata and fs here. because e.g.
		 * if the user tries to mount the device "/realfile" the userspace has no opportunity
		 * to distinguish between virtual and real files. therefore fs will try to open this
		 * path and shoot itself in the foot... */
		if(pid == ATA_PID || pid == FS_PID)
			return ERR_PATH_NOT_FOUND;

		/* send msg to fs and wait for reply */
		file = vfs_real_openPath(pid,flags,path);
		if(file < 0)
			return file;
	}
	else {
		node = vfs_node_get(nodeNo);
		/* handle virtual files */
		if(err < 0)
			return err;
		/* store whether we have created the node */
		if(created)
			flags |= VFS_CREATED;

		/* if its a driver, create the usage-node */
		if(IS_DRIVER(node->mode)) {
			sVFSNode *child = vfs_chan_create(pid,node);
			if(child == NULL)
				return ERR_NOT_ENOUGH_MEM;
			node = child;
			nodeNo = vfs_node_getNo(node);
		}

		/* open file */
		file = vfs_openFile(pid,flags,nodeNo,VFS_DEV_NO);
		if(file < 0)
			return file;

		/* if it is a driver, call the driver open-command */
		if(IS_DRVUSE(node->mode)) {
			err = vfsdrv_open(pid,file,node,flags);
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

tFileNo vfs_openFile(tPid pid,u16 flags,tInodeNo nodeNo,tDevNo devNo) {
	sGFTEntry *e;
	sVFSNode *n = NULL;
	tFileNo f;

	/* cleanup flags */
	flags &= VFS_READ | VFS_WRITE | VFS_NOBLOCK | VFS_CREATED;

	/* determine free file */
	f = vfs_getFreeFile(pid,flags,nodeNo,devNo);
	if(f < 0)
		return f;

	if(devNo == VFS_DEV_NO) {
		s32 err;
		n = vfs_node_get(nodeNo);
		if((err = vfs_hasAccess(pid,nodeNo,flags)) < 0)
			return err;
	}

	/* unused file? */
	e = globalFileTable + f;
	if(e->flags == 0) {
		/* count references of virtual nodes */
		if(devNo == VFS_DEV_NO)
			n->refCount++;
		e->owner = pid;
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

static tFileNo vfs_getFreeFile(tPid pid,u16 flags,tInodeNo nodeNo,tDevNo devNo) {
	tFileNo i;
	tFileNo freeSlot = ERR_NO_FREE_FILE;
	u16 rwFlags = flags & (VFS_READ | VFS_WRITE | VFS_NOBLOCK);
	bool isDrvUse = false;
	sGFTEntry *e = globalFileTable;

	/* ensure that we don't increment usages of an unused slot */
	assert(flags & (VFS_READ | VFS_WRITE));
	assert(!(flags & ~(VFS_READ | VFS_WRITE | VFS_NOBLOCK | VFS_CREATED)));

	if(devNo == VFS_DEV_NO) {
		sVFSNode *n = vfs_node_get(nodeNo);
		assert(vfs_node_isValid(nodeNo));
		/* we can add pipes here, too, since every open() to a pipe will get a new node anyway */
		isDrvUse = (n->mode & (MODE_TYPE_DRVUSE | MODE_TYPE_PIPE)) ? true : false;
	}

	for(i = 0; i < FILE_COUNT; i++) {
		/* used slot and same node? */
		if(e->flags != 0) {
			/* same file? */
			if(e->devNo == devNo && e->nodeNo == nodeNo) {
				if(e->owner == pid) {
					/* if the flags are the same we don't need a new file */
					if((e->flags & (VFS_READ | VFS_WRITE | VFS_NOBLOCK)) == rwFlags)
						return i;
				}
				/* two procs that want to write at the same time? no! */
				else if(!isDrvUse && (rwFlags & VFS_WRITE) && (e->flags & VFS_WRITE))
					return ERR_FILE_IN_USE;
			}
		}
		/* remember free slot */
		else if(freeSlot == ERR_NO_FREE_FILE) {
			freeSlot = i;
			/* just for performance: if we've found an unused file and want to use a driver,
			 * use this slot because it doesn't really matter whether we use a new file or an
			 * existing one (if there even is any) */
			/* note: we can share a file for writing in this case! */
			/* when we don't write, we don't need to wait since it doesn't hurt if there are other
			 * users of that file */
			if(isDrvUse || !(rwFlags & VFS_WRITE))
				break;
		}

		e++;
	}
	/* if there is no free slot anymore, extend our dyn-array */
	if(freeSlot == ERR_NO_FREE_FILE) {
		if(!dyna_extend(&gftArray))
			return ERR_NO_FREE_FILE;
		freeSlot = i;
	}

	return freeSlot;
}

u32 vfs_tell(tPid pid,tFileNo file) {
	UNUSED(pid);
	sGFTEntry *e = globalFileTable + file;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);
	return e->position;
}

s32 vfs_stat(tPid pid,const char *path,sFileInfo *info) {
	tInodeNo nodeNo;
	s32 res = vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH)
		res = vfs_real_stat(pid,path,info);
	else if(res == 0)
		res = vfs_node_getInfo(nodeNo,info);
	return res;
}

s32 vfs_fstat(tPid pid,tFileNo file,sFileInfo *info) {
	sGFTEntry *e = globalFileTable + file;
	s32 res;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);
	if(e->devNo == VFS_DEV_NO)
		res = vfs_node_getInfo(e->nodeNo,info);
	else
		res = vfs_real_istat(pid,e->nodeNo,e->devNo,info);
	return res;
}

bool vfs_isterm(tPid pid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfs_node_get(e->nodeNo);
	UNUSED(pid);
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);
	if(e->devNo != VFS_DEV_NO)
		return false;
	/* node not present anymore */
	if(n->name == NULL)
		return false;
	return IS_DRVUSE(n->mode) && vfs_server_isterm(n->parent);
}

s32 vfs_seek(tPid pid,tFileNo file,s32 offset,u32 whence) {
	sGFTEntry *e = globalFileTable + file;
	u32 oldPos = e->position;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = vfs_node_get(e->nodeNo);
		/* node not present anymore */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
		if(n->seek == NULL)
			return ERR_UNSUPPORTED_OP;
		e->position = n->seek(pid,n,e->position,offset,whence);
	}
	else {
		if(whence == SEEK_END) {
			sFileInfo info;
			s32 res = vfs_real_istat(pid,e->nodeNo,e->devNo,&info);
			if(res < 0)
				return res;
			e->position = info.size;
		}
		/* since the fs-driver validates the position anyway we can simply set it */
		else if(whence == SEEK_SET)
			e->position = offset;
		else
			e->position += offset;
	}

	if((s32)e->position < 0) {
		s32 err = e->position;
		e->position = oldPos;
		return err;
	}
	return e->position;
}

s32 vfs_readFile(tPid pid,tFileNo file,u8 *buffer,u32 count) {
	s32 err,readBytes;
	sGFTEntry *e = globalFileTable + file;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(!(e->flags & VFS_READ))
		return ERR_NO_READ_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = vfs_node_get(e->nodeNo);
		if((err = vfs_hasAccess(pid,e->nodeNo,VFS_READ)) < 0)
			return err;
		if(n->readHandler == NULL)
			return ERR_NO_READ_PERM;

		/* use the read-handler */
		readBytes = n->readHandler(pid,file,n,buffer,e->position,count);
		if(readBytes > 0)
			e->position += readBytes;
	}
	else {
		/* query the fs-driver to read from the inode */
		readBytes = vfs_real_read(pid,e->nodeNo,e->devNo,buffer,e->position,count);
		if(readBytes > 0)
			e->position += readBytes;
	}

	if(readBytes > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		p->stats.input += readBytes;
	}
	return readBytes;
}

s32 vfs_writeFile(tPid pid,tFileNo file,const u8 *buffer,u32 count) {
	s32 err,writtenBytes;
	sGFTEntry *e = globalFileTable + file;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(!(e->flags & VFS_WRITE))
		return ERR_NO_WRITE_PERM;

	if(e->devNo == VFS_DEV_NO) {
		sVFSNode *n = vfs_node_get(e->nodeNo);
		if((err = vfs_hasAccess(pid,e->nodeNo,VFS_WRITE)) < 0)
			return err;
		if(n->writeHandler == NULL)
			return ERR_NO_WRITE_PERM;

		/* write to the node */
		writtenBytes = n->writeHandler(pid,file,n,buffer,e->position,count);
		if(writtenBytes > 0) {
			e->flags |= VFS_MODIFIED;
			e->position += writtenBytes;
		}
	}
	else {
		/* query the fs-driver to write to the inode */
		writtenBytes = vfs_real_write(pid,e->nodeNo,e->devNo,buffer,e->position,count);
		if(writtenBytes > 0)
			e->position += writtenBytes;
	}

	if(writtenBytes > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		p->stats.output += writtenBytes;
	}
	return writtenBytes;
}

s32 vfs_sendMsg(tPid pid,tFileNo file,tMsgId id,const u8 *data,u32 size) {
	s32 err;
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO)
		return ERR_INVALID_FILE;
	if((err = vfs_hasAccess(pid,e->nodeNo,VFS_WRITE)) < 0)
		return err;

	/* send the message */
	n = vfs_node_get(e->nodeNo);
	if(!IS_DRVUSE(n->mode))
		return ERR_UNSUPPORTED_OP;
	err = vfs_chan_send(pid,file,n,id,data,size);

	if(err == 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		p->stats.output += size;
	}
	return err;
}

s32 vfs_receiveMsg(tPid pid,tFileNo file,tMsgId *id,u8 *data,u32 size) {
	s32 err;
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO)
		return ERR_INVALID_FILE;

	if((err = vfs_hasAccess(pid,e->nodeNo,VFS_READ)) < 0)
		return err;

	/* receive the message */
	n = vfs_node_get(e->nodeNo);
	if(!IS_DRVUSE(n->mode))
		return ERR_UNSUPPORTED_OP;
	err = vfs_chan_receive(pid,file,n,id,data,size);

	if(err > 0 && pid != KERNEL_PID) {
		sProc *p = proc_getByPid(pid);
		p->stats.input += err;
	}
	return err;
}

void vfs_closeFile(tPid pid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	/* decrement references */
	if(--(e->refCount) == 0) {
		if(e->devNo == VFS_DEV_NO) {
			sVFSNode *n = vfs_node_get(e->nodeNo);
			if(n->name != NULL) {
				n->refCount--;
				if(n->close)
					n->close(pid,file,n);
			}
		}
		else
			vfs_real_close(pid,e->nodeNo,e->devNo);

		/* mark unused */
		e->flags = 0;
	}
}

s32 vfs_link(tPid pid,const char *oldPath,const char *newPath) {
	char newPathCpy[MAX_PATH_LEN];
	char *name,*namecpy,backup;
	u32 len;
	tInodeNo oldIno,newIno;
	sVFSNode *dir,*target;
	s32 oldRes,newRes;
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

	/* TODO check access-rights */

	/* copy path because we have to change it */
	len = strlen(newPath);
	if(len >= MAX_PATH_LEN)
		return ERR_INVALID_PATH;
	strcpy(newPathCpy,newPath);
	/* check whether the directory exists */
	name = vfs_node_basename((char*)newPathCpy,&len);
	backup = *name;
	vfs_node_dirname((char*)newPathCpy,len);
	newRes = vfs_node_resolvePath(newPathCpy,&newIno,NULL,VFS_WRITE);
	if(newRes < 0)
		return ERR_PATH_NOT_FOUND;

	/* links to directories not allowed */
	target = vfs_node_get(oldIno);
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
	dir = vfs_node_get(newIno);
	/* file exists? */
	if(vfs_node_findInDir(dir,namecpy,len) != NULL) {
		kheap_free(namecpy);
		return ERR_FILE_EXISTS;
	}
	if(vfs_link_create(pid,dir,namecpy,target) == NULL) {
		kheap_free(namecpy);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 vfs_unlink(tPid pid,const char *path) {
	s32 res;
	tInodeNo ino;
	sVFSNode *n;
	res = vfs_node_resolvePath(path,&ino,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == ERR_REAL_PATH)
		return vfs_real_unlink(pid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;
	/* TODO check access-rights */
	n = vfs_node_get(ino);
	if(!MODE_IS_FILE(n->mode) && !MODE_IS_LINK(n->mode))
		return ERR_NO_FILE_OR_LINK;
	vfs_node_destroy(n);
	return 0;
}

s32 vfs_mkdir(tPid pid,const char *path) {
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
	namecpy = kheap_alloc(len + 1);
	if(namecpy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strcpy(namecpy,name);
	/* create dir */
	node = vfs_node_get(inodeNo);
	if(vfs_node_findInDir(node,namecpy,len) != NULL) {
		kheap_free(namecpy);
		return ERR_FILE_EXISTS;
	}
	/* TODO check access-rights */
	child = vfs_dir_create(pid,node,namecpy);
	if(child == NULL) {
		kheap_free(namecpy);
		return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 vfs_rmdir(tPid pid,const char *path) {
	s32 res;
	sVFSNode *node;
	tInodeNo inodeNo;
	res = vfs_node_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == ERR_REAL_PATH)
		return vfs_real_rmdir(pid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;

	/* TODO check access-rights */
	node = vfs_node_get(inodeNo);
	if(!MODE_IS_DIR(node->mode))
		return ERR_NO_DIRECTORY;
	vfs_node_destroy(node);
	return 0;
}

tFileNo vfs_createDriver(tPid pid,const char *name,u32 flags) {
	sVFSNode *drv = devNode;
	sVFSNode *n = drv->firstChild;
	u32 len;
	char *hname;
	vassert(name != NULL,"name == NULL");

	/* we don't want to have exotic driver-names */
	if((len = strlen(name)) == 0 || !isalnumstr(name))
		return ERR_INV_DRIVER_NAME;

	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			return ERR_DRIVER_EXISTS;
		n = n->next;
	}

	/* copy name to kernel-heap */
	hname = (char*)kheap_alloc(len + 1);
	if(hname == NULL)
		return ERR_NOT_ENOUGH_MEM;
	strncpy(hname,name,len);
	hname[len] = '\0';

	/* create node */
	n = vfs_server_create(pid,drv,hname,flags);
	if(n != NULL)
		return vfs_openFile(pid,VFS_READ,vfs_node_getNo(n),VFS_DEV_NO);

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

bool vfs_hasMsg(tPid pid,tFileNo file) {
	UNUSED(pid);
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	assert(file >= 0 && file < FILE_COUNT);
	if(e->devNo != VFS_DEV_NO)
		return false;
	n = vfs_node_get(e->nodeNo);
	return n->name != NULL && IS_DRVUSE(n->mode) && vfs_chan_hasReply(n);
}

bool vfs_hasData(tPid pid,tFileNo file) {
	UNUSED(pid);
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	assert(file >= 0 && file < FILE_COUNT);
	if(e->devNo != VFS_DEV_NO)
		return false;
	n = vfs_node_get(e->nodeNo);
	return n->name != NULL && IS_DRIVER(n->parent->mode) && vfs_server_isReadable(n->parent);
}

void vfs_wakeupClients(sVFSNode *node,u32 events) {
	assert(IS_DRIVER(node->mode));
	node = vfs_node_getFirstChild(node);
	while(node != NULL) {
		ev_wakeup(events,(tEvObj)node);
		node = node->next;
	}
}

s32 vfs_getClient(tPid pid,tFileNo *files,u32 count,s32 *index) {
	sVFSNode *node = NULL,*client,*match = NULL;
	u32 i;
	bool retry,cont = true;
start:
	retry = false;
	for(i = 0; cont && i < count; i++) {
		sGFTEntry *e = globalFileTable + files[i];
		if(files[i] < 0 || files[i] >= FILE_COUNT || e->devNo != VFS_DEV_NO)
			return ERR_INVALID_FILE;

		node = vfs_node_get(e->nodeNo);
		if(node->owner != pid || !IS_DRIVER(node->mode))
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
	if(retry)
		goto start;
	return ERR_NO_CLIENT_WAITING;
}

tInodeNo vfs_getClientId(tPid pid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfs_node_get(e->nodeNo);
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO || e->owner != pid)
		return ERR_INVALID_FILE;
	if(!IS_DRVUSE(n->mode))
		return ERR_INVALID_FILE;
	return e->nodeNo;
}

tFileNo vfs_openClient(tPid pid,tFileNo file,tInodeNo clientId) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	/* search for the client */
	n = vfs_node_getFirstChild(vfs_node_get(e->nodeNo));
	while(n != NULL) {
		if(vfs_node_getNo(n) == clientId)
			break;
		n = n->next;
	}
	if(n == NULL)
		return ERR_PATH_NOT_FOUND;

	/* open file */
	return vfs_openFile(pid,VFS_READ | VFS_WRITE,vfs_node_getNo(n),VFS_DEV_NO);
}

sVFSNode *vfs_createProcess(tPid pid,fRead handler) {
	char *name;
	sVFSNode *proc = procsNode;
	sVFSNode *n = proc->firstChild;
	sVFSNode *dir,*tdir;

	/* build name */
	name = (char*)kheap_alloc(12);
	if(name == NULL)
		return NULL;

	itoa(name,12,pid);

	/* go to last entry */
	while(n != NULL) {
		/* entry already existing? */
		if(strcmp(n->name,name) == 0)
			goto errorName;
		n = n->next;
	}

	/* create dir */
	dir = vfs_dir_create(KERNEL_PID,proc,name);
	if(dir == NULL)
		goto errorName;

	/* create process-info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",handler,NULL);
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

	return tdir;

errorDir:
	vfs_node_destroy(dir);
errorName:
	kheap_free(name);
	return NULL;
}

void vfs_removeProcess(tPid pid) {
	sVFSNode *rn = devNode;
	sVFSNode *n,*tn;
	sProc *p = proc_getByPid(pid);

	/* check if the process has a driver */
	n = vfs_node_getFirstChild(rn->firstChild);
	while(n != NULL) {
		if(IS_DRIVER(n->mode) && n->owner == pid) {
			tn = n->next;
			vfs_node_destroy(n);
			n = tn;
		}
		else
			n = n->next;
	}

	/* remove from /system/processes */
	vfs_node_destroy(p->threadDir->parent);
}

bool vfs_createThread(tTid tid) {
	char *name;
	sVFSNode *n,*dir;
	sThread *t = thread_getById(tid);

	/* build name */
	name = (char*)kheap_alloc(12);
	if(name == NULL)
		return false;
	itoa(name,12,tid);

	/* create dir */
	dir = vfs_dir_create(KERNEL_PID,t->proc->threadDir,name);
	if(dir == NULL)
		goto errorDir;

	/* create info-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"info",vfs_info_threadReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;

	/* create trace-node */
	n = vfs_file_create(KERNEL_PID,dir,(char*)"trace",vfs_info_traceReadHandler,NULL);
	if(n == NULL)
		goto errorInfo;
	return true;

errorInfo:
	vfs_node_destroy(dir);
errorDir:
	kheap_free(name);
	return false;
}

void vfs_removeThread(tTid tid) {
	sThread *t = thread_getById(tid);
	sVFSNode *n;
	char *name;

	/* build name */
	name = (char*)kheap_alloc(12);
	if(name == NULL)
		return;
	itoa(name,12,tid);

	/* search for thread-node and remove it */
	n = vfs_node_getFirstChild(t->proc->threadDir);
	while(n != NULL) {
		if(strcmp(n->name,name) == 0) {
			vfs_node_destroy(n);
			break;
		}
		n = n->next;
	}

	kheap_free(name);
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

u32 vfs_dbg_getGFTEntryCount(void) {
	s32 i;
	u32 count = 0;
	for(i = 0; i < FILE_COUNT; i++) {
		if(globalFileTable[i].flags != 0)
			count++;
	}
	return count;
}

void vfs_dbg_printMsgs(void) {
	sVFSNode *drv = vfs_node_getFirstChild(devNode);
	vid_printf("Messages:\n");
	while(drv != NULL) {
		if(IS_DRIVER(drv->mode))
			vfs_server_dbg_print(drv);
		drv = drv->next;
	}
}

void vfs_dbg_printGFT(void) {
	tFileNo i;
	sGFTEntry *e = globalFileTable;
	vid_printf("Global File Table:\n");
	for(i = 0; i < FILE_COUNT; i++) {
		if(e->flags != 0) {
			vid_printf("\tfile @ index %d\n",i);
			vid_printf("\t\tflags: ");
			if(e->flags & VFS_READ)
				vid_printf("READ ");
			if(e->flags & VFS_WRITE)
				vid_printf("WRITE ");
			if(e->flags & VFS_NOBLOCK)
				vid_printf("NOBLOCK ");
			vid_printf("\n");
			vid_printf("\t\tnodeNo: %d\n",e->nodeNo);
			vid_printf("\t\tdevNo: %d\n",e->devNo);
			vid_printf("\t\tpos: %d\n",e->position);
			vid_printf("\t\trefCount: %d\n",e->refCount);
			if(e->owner == KERNEL_PID)
				vid_printf("\t\towner: %d (kernel)\n",e->owner);
			else {
				sProc *p = proc_getByPid(e->owner);
				vid_printf("\t\towner: %d:%s\n",p->pid,p->command);
			}
			if(e->devNo == VFS_DEV_NO) {
				sVFSNode *n = vfs_node_get(e->nodeNo);
				if(IS_DRVUSE(n->mode))
					vid_printf("\t\tDriver-Usage: %s @ %s\n",n->name,n->parent->name);
				else
					vid_printf("\t\tFile: '%s'\n",vfs_node_getPath(vfs_node_getNo(n)));
			}
		}
		e++;
	}
}

#endif
