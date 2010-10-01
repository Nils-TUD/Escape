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
#include <sys/vfs/rw.h>
#include <sys/vfs/listeners.h>
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

/* the processes node */
#define PROCESSES()					(vfsn_getNode(7))
/* the drivers-node */
#define DRIVERS()					(vfsn_getNode(13))

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

void vfs_init(void) {
	sVFSNode *root,*sys;
	dyna_init(&gftArray,sizeof(sGFTEntry),GFT_AREA,GFT_AREA_SIZE);
	vfsn_init();

	/*
	 *  /
	 *   system
	 *     |-pipe
	 *     |-processes
	 *     |-devices
	 *     |-bin
	 *   dev
	 */
	root = vfsn_createDir(NULL,(char*)"");
	sys = vfsn_createDir(root,(char*)"system");
	vfsn_createPipeCon(sys,(char*)"pipe");
	vfsn_createDir(sys,(char*)"processes");
	vfsn_createDir(sys,(char*)"devices");
	vfsn_createDir(root,(char*)"dev");
}

s32 vfs_hasAccess(tPid pid,tInodeNo nodeNo,u16 flags) {
	sVFSNode *n = vfsn_getNode(nodeNo);
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

s32 vfs_fcntl(tPid pid,tFileNo file,u32 cmd,s32 arg) {
	sGFTEntry *e = globalFileTable + file;
	assert(file >= 0 && file < FILE_COUNT);
	switch(cmd) {
		case F_GETFL:
			return e->flags;
		case F_SETFL:
			e->flags &= VFS_READ | VFS_WRITE | VFS_CREATE;
			e->flags |= arg & VFS_NOBLOCK;
			return 0;
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
	s32 err = vfsn_resolvePath(path,&nodeNo,&created,flags);
	if(err == ERR_REAL_PATH) {
		/* unfortunatly we have to check for the process-ids of ata and fs here. because e.g.
		 * if the user tries to mount the device "/realfile" the userspace has no opportunity
		 * to distinguish between virtual and real files. therefore fs will try to open this
		 * path and shoot itself in the foot... */
		if(pid == ATA_PID || pid == FS_PID)
			return ERR_PATH_NOT_FOUND;

		/* send msg to fs and wait for reply */
		file = vfsr_openFile(pid,flags,path);
		if(file < 0)
			return file;
	}
	else {
		node = vfsn_getNode(nodeNo);
		/* handle virtual files */
		if(err < 0)
			return err;
		/* store whether we have created the node */
		if(created)
			flags |= VFS_CREATED;

		/* if its a driver, create the usage-node */
		if(IS_DRIVER(node->mode)) {
			sVFSNode *child;
			err = vfsn_createUse(pid,node,&child);
			if(err < 0)
				return err;
			node = child;
			nodeNo = vfsn_getNodeNo(node);
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
		n = vfsn_getNode(nodeNo);
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
		sVFSNode *n = vfsn_getNode(nodeNo);
		assert(vfsn_isValidNodeNo(nodeNo));
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
	s32 res = vfsn_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH)
		res = vfsr_stat(pid,path,info);
	else if(res == 0)
		res = vfsn_getNodeInfo(nodeNo,info);
	return res;
}

s32 vfs_fstat(tPid pid,tFileNo file,sFileInfo *info) {
	sGFTEntry *e = globalFileTable + file;
	s32 res;
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);
	if(e->devNo == VFS_DEV_NO)
		res = vfsn_getNodeInfo(e->nodeNo,info);
	else
		res = vfsr_istat(pid,e->nodeNo,e->devNo,info);
	return res;
}

bool vfs_isterm(tPid pid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfsn_getNode(e->nodeNo);
	UNUSED(pid);
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);
	if(e->devNo != VFS_DEV_NO)
		return false;
	/* node not present anymore */
	if(n->name == NULL)
		return false;
	return IS_DRVUSE(n->mode) && (n->parent->data.driver.funcs & DRV_TERM);
}

s32 vfs_seek(tPid pid,tFileNo file,s32 offset,u32 whence) {
	sGFTEntry *e = globalFileTable + file;
	s32 newPos;
	UNUSED(pid);
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

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
		sVFSNode *n = vfsn_getNode(e->nodeNo);
		/* node not present anymore */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
		/* no seek for pipes */
		if(IS_PIPE(n->mode))
			return ERR_PIPE_SEEK;

		if(IS_DRVUSE(n->mode)) {
			/* not supported for drivers */
			if(whence == SEEK_END)
				return ERR_INVALID_ARGS;
			e->position = newPos;
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
			s32 res = vfsr_istat(pid,e->nodeNo,e->devNo,&info);
			if(res < 0)
				return res;
			e->position = info.size;
		}
		else {
			/* since the fs-driver validates the position anyway we can simply set it */
			e->position = newPos;
		}
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
		sVFSNode *n = vfsn_getNode(e->nodeNo);

		if((err = vfs_hasAccess(pid,e->nodeNo,VFS_READ)) < 0)
			return err;

		/* node not present anymore or no read-handler? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
		if(n->readHandler == NULL)
			return ERR_NO_READ_PERM;

		/* use the read-handler */
		readBytes = n->readHandler(pid,file,n,buffer,e->position,count);
		if(readBytes > 0)
			e->position += readBytes;
	}
	else {
		/* query the fs-driver to read from the inode */
		readBytes = vfsr_readFile(pid,e->nodeNo,e->devNo,buffer,e->position,count);
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
		sVFSNode *n = vfsn_getNode(e->nodeNo);

		if((err = vfs_hasAccess(pid,e->nodeNo,VFS_WRITE)) < 0)
			return err;

		/* node not present anymore or no write-handler? */
		if(n->name == NULL)
			return ERR_INVALID_FILE;
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
		writtenBytes = vfsr_writeFile(pid,e->nodeNo,e->devNo,buffer,e->position,count);
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

	n = vfsn_getNode(e->nodeNo);

	if((err = vfs_hasAccess(pid,e->nodeNo,VFS_WRITE)) < 0)
		return err;

	/* node not present anymore? */
	if(n->name == NULL)
		return ERR_INVALID_FILE;

	/* send the message */
	err = vfsrw_writeDrvUse(pid,file,n,id,data,size);

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

	n = vfsn_getNode(e->nodeNo);

	if((err = vfs_hasAccess(pid,e->nodeNo,VFS_READ)) < 0)
		return err;

	/* node not present anymore? */
	if(n->name == NULL)
		return ERR_INVALID_FILE;

	/* receive the message */
	err = vfsrw_readDrvUse(pid,file,n,id,data,size);

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
			sVFSNode *n = vfsn_getNode(e->nodeNo);
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
					if(IS_DRVUSE(n->mode))
						vfsdrv_close(pid,file,n);

					/* remove pipe-nodes if there are no references anymore */
					if(IS_PIPE(n->mode))
						vfsn_removeNode(n);
					/* if there are message for the driver we don't want to throw them away */
					/* if there are any in the receivelist (and no references of the node) we
					 * can throw them away because no one will read them anymore
					 * (it means that the client has already closed the file) */
					/* note also that we can assume that the driver is still running since we
					 * would have deleted the whole driver-node otherwise */
					else if(IS_DRVUSE(n->mode) && sll_length(n->data.drvuse.sendList) == 0)
						vfsn_removeNode(n);
				}
				/* there are still references to the pipe, write an EOF into it (0 references
				 * mean that the pipe should be removed)
				 */
				else if(IS_PIPE(n->mode))
					vfsrw_writePipe(pid,file,n,NULL,e->position,0);
				/* TODO perhaps we should notify the writer if the read has closed the file
				 * (e.g. by a kill) */
			}
		}
		else
			vfsr_closeFile(pid,e->nodeNo,e->devNo);

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
	oldRes = vfsn_resolvePath(oldPath,&oldIno,NULL,VFS_READ);
	newRes = vfsn_resolvePath(newPath,&newIno,NULL,VFS_WRITE);
	if(oldRes == ERR_REAL_PATH) {
		if(newRes != ERR_REAL_PATH)
			return ERR_LINK_DEVICE;
		return vfsr_link(pid,oldPath,newPath);
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

s32 vfs_unlink(tPid pid,const char *path) {
	s32 res;
	tInodeNo ino;
	sVFSNode *n;
	res = vfsn_resolvePath(path,&ino,NULL,VFS_WRITE | VFS_NOLINKRES);
	if(res == ERR_REAL_PATH)
		return vfsr_unlink(pid,path);
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
	name = vfsn_basename(pathCpy,&len);
	backup = *name;
	vfsn_dirname(pathCpy,len);

	/* get the parent-directory */
	res = vfsn_resolvePath(pathCpy,&inodeNo,NULL,VFS_WRITE);
	/* special-case: directories in / should be created in the real fs! */
	if(res == ERR_REAL_PATH || (res >= 0 && strcmp(pathCpy,"/") == 0)) {
		/* let fs handle the request */
		return vfsr_mkdir(pid,path);
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

s32 vfs_rmdir(tPid pid,const char *path) {
	s32 res;
	sVFSNode *node;
	tInodeNo inodeNo;
	res = vfsn_resolvePath(path,&inodeNo,NULL,VFS_WRITE);
	if(res == ERR_REAL_PATH)
		return vfsr_rmdir(pid,path);
	if(res < 0)
		return ERR_PATH_NOT_FOUND;

	/* TODO check access-rights */
	node = vfsn_getNode(inodeNo);
	if(!MODE_IS_DIR(node->mode))
		return ERR_NO_DIRECTORY;
	vfsn_removeNode(node);
	return 0;
}

tDrvId vfs_createDriver(tPid pid,const char *name,u32 flags) {
	sVFSNode *drv = DRIVERS();
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
	n = vfsn_createDriverNode(pid,drv,hname,flags);
	if(n != NULL)
		return vfsn_getNodeNo(n);

	/* failed, so cleanup */
	kheap_free(hname);
	return ERR_NOT_ENOUGH_MEM;
}

s32 vfs_setDataReadable(tPid pid,tInodeNo nodeNo,bool readable) {
	sVFSNode *n = vfsn_getNode(nodeNo);
	if(n->name == NULL || !IS_DRIVER(n->mode) || !DRV_IMPL(n->data.driver.funcs,DRV_READ))
		return ERR_INVALID_DRVID;
	if(n->owner != pid)
		return ERR_NOT_OWN_DRIVER;

	n->data.driver.isEmpty = !readable;
	if(readable) {
		vfs_wakeupClients(n,EVI_RECEIVED_MSG);
		ev_wakeup(EVI_DATA_READABLE,n);
	}
	return 0;
}

bool vfs_msgAvailableFor(tPid pid,u32 events) {
	sVFSNode *n;
	sProc *p = proc_getByPid(pid);
	tFD i;

	/* at first we check whether the process is a driver */
	if(events & EV_CLIENT) {
		sVFSNode *rn = DRIVERS();
		n = vfsn_getFirstChild(rn);
		while(n != NULL) {
			if(n->owner == pid) {
				tInodeNo nodeNo = vfsn_getNodeNo(n);
				tInodeNo client = vfs_getClient(pid,&nodeNo,1);
				if(vfsn_isValidNodeNo(client))
					return true;
			}
			n = n->next;
		}
	}

	/* now search through the file-descriptors if there is any message */
	if(events & EV_RECEIVED_MSG) {
		for(i = 0; i < MAX_FD_COUNT; i++) {
			if(p->fileDescs[i] != -1) {
				sGFTEntry *e = globalFileTable + p->fileDescs[i];
				if(e->devNo == VFS_DEV_NO) {
					n = vfsn_getNode(e->nodeNo);
					/* driver-usage and a message in the receive-list? */
					/* we don't want to check that if it is our own driver. because it makes no
					 * sense to read from ourself ;) */
					if(IS_DRVUSE(n->mode) && n->parent->owner != pid) {
						if(sll_length(n->data.drvuse.recvList) > 0)
							return true;
					}
				}
			}
		}
	}

	return false;
}

void vfs_wakeupClients(sVFSNode *node,u32 events) {
	assert(IS_DRIVER(node->mode));
	node = vfsn_getFirstChild(node);
	while(node != NULL) {
		ev_wakeup(events,node);
		node = node->next;
	}
}

s32 vfs_getClient(tPid pid,tInodeNo *vfsNodes,u32 count) {
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
			return ERR_INVALID_DRVID;

		node = vfsn_getNode(vfsNodes[i]);
		if(node->owner != pid || !IS_DRIVER(node->mode))
			return ERR_NOT_OWN_DRIVER;

		/* search for a slot that needs work */
		last = node->data.driver.lastClient;
		if(last == NULL)
			n = vfsn_getFirstChild(node);
		else if(last->next == NULL) {
			/* if we have checked all clients in this driver, give the other drivers
			 * a chance (if there are any others) */
			node->data.driver.lastClient = NULL;
			skipped = true;
			continue;
		}
		else
			n = last->next;
searchBegin:
		while(n != NULL && n != last) {
			/* data available? */
			if(sll_length(n->data.drvuse.sendList) > 0) {
				node->data.driver.lastClient = n;
				return vfsn_getNodeNo(n);
			}
			n = n->next;
		}
		/* if we have looked through all nodes and the last one has a message again, we have to
		 * store it because we'll not check it again */
		if(last && n == last && sll_length(n->data.drvuse.sendList) > 0)
			match = n;
		else if(node->data.driver.lastClient) {
			n = vfsn_getFirstChild(node);
			node->data.driver.lastClient = NULL;
			goto searchBegin;
		}
	}
	/* if we have a match, use this one */
	if(match) {
		node->data.driver.lastClient = match;
		return vfsn_getNodeNo(match);
	}
	/* if not and we've skipped a client, try another time */
	if(skipped)
		goto retry;
	return ERR_NO_CLIENT_WAITING;
}

tInodeNo vfs_getClientId(tPid pid,tFileNo file) {
	sGFTEntry *e = globalFileTable + file;
	sVFSNode *n = vfsn_getNode(e->nodeNo);
	vassert(file >= 0 && file < FILE_COUNT && e->flags != 0,"Invalid file %d",file);

	if(e->devNo != VFS_DEV_NO || e->owner != pid)
		return ERR_INVALID_FILE;
	if(!IS_DRVUSE(n->mode))
		return ERR_INVALID_FILE;
	return e->nodeNo;
}

tFileNo vfs_openClient(tPid pid,tInodeNo nodeNo,tInodeNo clientId) {
	sVFSNode *n,*node;
	/* check if the node is valid */
	if(!vfsn_isValidNodeNo(nodeNo))
		return ERR_INVALID_DRVID;
	node = vfsn_getNode(nodeNo);
	if(node->owner != pid || !IS_DRIVER(node->mode))
		return ERR_NOT_OWN_DRIVER;

	/* search for a slot that needs work */
	n = vfsn_getFirstChild(node);
	while(n != NULL) {
		if(vfsn_getNodeNo(n) == clientId)
			break;
		n = n->next;
	}

	/* not found? */
	if(n == NULL)
		return ERR_PATH_NOT_FOUND;

	/* open file */
	return vfs_openFile(pid,VFS_READ | VFS_WRITE,vfsn_getNodeNo(n),VFS_DEV_NO);
}

s32 vfs_removeDriver(tPid pid,tInodeNo nodeNo) {
	sVFSNode *n = vfsn_getNode(nodeNo);

	vassert(vfsn_isValidNodeNo(nodeNo),"Invalid node number %d",nodeNo);

	if(n->owner != pid || !IS_DRIVER(n->mode))
		return ERR_NOT_OWN_DRIVER;

	/* wakeup all threads that may be waiting for this node so they can check
	 * whether they are affected by the remove of this driver and perform the corresponding
	 * action */
	vfs_wakeupClients(n,EVI_RECEIVED_MSG);
	vfs_wakeupClients(n,EVI_REQ_REPLY);
	ev_wakeup(EVI_DATA_READABLE,n);

	/* remove driver-node including all driver-usages */
	vfsn_removeNode(n);
	return 0;
}

sVFSNode *vfs_createProcess(tPid pid,fRead handler) {
	char *name;
	sVFSNode *proc = PROCESSES();
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
	dir = vfsn_createDir(proc,name);
	if(dir == NULL)
		goto errorName;

	/* create process-info-node */
	n = vfsn_createFile(KERNEL_PID,dir,(char*)"info",handler,NULL,true);
	if(n == NULL)
		goto errorDir;

	/* create virt-mem-info-node */
	n = vfsn_createFile(KERNEL_PID,dir,(char*)"virtmem",vfsinfo_virtMemReadHandler,NULL,true);
	if(n == NULL)
		goto errorDir;

	/* create regions-info-node */
	n = vfsn_createFile(KERNEL_PID,dir,(char*)"regions",vfsinfo_regionsReadHandler,NULL,true);
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
	sVFSNode *rn = DRIVERS();
	sVFSNode *n,*tn;
	sProc *p = proc_getByPid(pid);

	/* check if the process has a driver */
	n = vfsn_getFirstChild(rn->firstChild);
	while(n != NULL) {
		if(IS_DRIVER(n->mode) && n->owner == pid) {
			tn = n->next;
			vfs_removeDriver(pid,vfsn_getNodeNo(n));
			n = tn;
		}
		else
			n = n->next;
	}

	/* remove from /system/processes */
	vfsn_removeNode(p->threadDir->parent);
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
	dir = vfsn_createDir(t->proc->threadDir,name);
	if(dir == NULL)
		goto errorDir;

	/* create info-node */
	n = vfsn_createFile(KERNEL_PID,dir,(char*)"info",vfsinfo_threadReadHandler,NULL,true);
	if(n == NULL)
		goto errorInfo;

	/* create trace-node */
	n = vfsn_createFile(KERNEL_PID,dir,(char*)"trace",vfsinfo_traceReadHandler,NULL,true);
	if(n == NULL)
		goto errorInfo;
	return true;

errorInfo:
	vfsn_removeNode(dir);
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
	n = vfsn_getFirstChild(t->proc->threadDir);
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
	s32 i;
	u32 count = 0;
	for(i = 0; i < FILE_COUNT; i++) {
		if(globalFileTable[i].flags != 0)
			count++;
	}
	return count;
}

void vfs_dbg_printMsgs(void) {
	sVFSNode *drv = vfsn_getFirstChild(DRIVERS());
	vid_printf("Messages:\n");
	while(drv != NULL) {
		if(IS_DRIVER(drv->mode))
			vfs_dbg_printMsgsOf(drv);
		drv = drv->next;
	}
}

void vfs_dbg_printMsgsOf(sVFSNode *n) {
	sVFSNode *chan = vfsn_getFirstChild(n);
	vid_printf("\t%s:\n",n->name);
	while(chan != NULL) {
		u32 i;
		sSLList *lists[] = {chan->data.drvuse.sendList,chan->data.drvuse.recvList};
		for(i = 0; i < ARRAY_SIZE(lists); i++) {
			u32 j,count = sll_length(lists[i]);
			vid_printf("\t\tChannel %s %s: (%d)\n",chan->name,i ? "recvs" : "sends",count);
			for(j = 0; j < count; j++)
				vfsrw_dbg_printMessage(sll_get(lists[i],j));
		}
		chan = chan->next;
	}
	vid_printf("\n");
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
				sVFSNode *n = vfsn_getNode(e->nodeNo);
				if(IS_DRVUSE(n->mode))
					vid_printf("\t\tDriver-Usage: %s @ %s\n",n->name,n->parent->name);
				else
					vid_printf("\t\tFile: '%s'\n",vfsn_getPath(vfsn_getNodeNo(n)));
			}
		}
		e++;
	}
}

#endif
