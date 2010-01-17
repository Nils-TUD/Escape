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
#include <mem/paging.h>
#include <vfs/real.h>
#include <vfs/driver.h>
#include <task/thread.h>
#include <syscalls/io.h>
#include <syscalls.h>
#include <errors.h>
#include <string.h>

void sysc_open(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	s32 pathLen;
	u16 flags;
	tInodeNo nodeNo = 0;
	bool created,isVirt = false;
	tFileNo file;
	tFD fd;
	s32 err;
	sThread *t = thread_getRunning();

	/* at first make sure that we'll cause no page-fault */
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	pathLen = strnlen(path,MAX_PATH_LEN);
	if(pathLen == -1)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check flags */
	flags = ((u16)SYSC_ARG2(stack)) & (VFS_WRITE | VFS_READ | VFS_CREATE | VFS_TRUNCATE | VFS_APPEND);
	if((flags & (VFS_READ | VFS_WRITE)) == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* resolve path */
	err = vfsn_resolvePath(path,&nodeNo,&created,flags | VFS_CONNECT);
	if(err == ERR_REAL_PATH) {
		/* send msg to fs and wait for reply */
		file = vfsr_openFile(t->tid,flags,path);
		if(file < 0)
			SYSC_ERROR(stack,file);

		/* get free fd */
		fd = thread_getFreeFd();
		if(fd < 0) {
			vfs_closeFile(t->tid,file);
			SYSC_ERROR(stack,fd);
		}
	}
	else {
		/* handle virtual files */
		if(err < 0)
			SYSC_ERROR(stack,err);

		/* get free fd */
		fd = thread_getFreeFd();
		if(fd < 0)
			SYSC_ERROR(stack,fd);
		/* store wether we have created the node */
		if(created)
			flags |= VFS_CREATED;
		/* open file */
		file = vfs_openFile(t->tid,flags,nodeNo,VFS_DEV_NO);
		if(file < 0)
			SYSC_ERROR(stack,file);
		isVirt = true;
	}

	/* assoc fd with file */
	err = thread_assocFd(fd,file);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* if it is a driver, call the driver open-command */
	if(isVirt) {
		sVFSNode *node = vfsn_getNode(nodeNo);
		if((node->mode & MODE_TYPE_SERVUSE) && IS_DRIVER(node->parent->mode)) {
			err = vfsdrv_open(t->tid,file,node,flags);
			/* if this went wrong, undo everything and report an error to the user */
			if(err < 0) {
				thread_unassocFd(fd);
				vfs_closeFile(t->tid,file);
				SYSC_ERROR(stack,err);
			}
		}
	}

	/* append? */
	if(flags & VFS_APPEND) {
		err = vfs_seek(t->tid,file,0,SEEK_END);
		if(err < 0)
			SYSC_ERROR(stack,err);
	}

	/* return fd */
	SYSC_RET1(stack,fd);
}

void sysc_pipe(sIntrptStackFrame *stack) {
	tFD *readFd = (tFD*)SYSC_ARG1(stack);
	tFD *writeFd = (tFD*)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	bool created;
	sVFSNode *pipeNode;
	tInodeNo nodeNo,pipeNodeNo;
	tFileNo readFile,writeFile;
	s32 err;

	/* check pointers */
	if(!paging_isRangeUserWritable((u32)readFd,sizeof(tFD)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((u32)writeFd,sizeof(tFD)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* resolve pipe-path */
	err = vfsn_resolvePath("/system/pipe",&nodeNo,&created,VFS_READ | VFS_CONNECT);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* create pipe */
	err = vfsn_createPipe(vfsn_getNode(nodeNo),&pipeNode);
	if(err < 0)
		SYSC_ERROR(stack,err);

	pipeNodeNo = NADDR_TO_VNNO(pipeNode);
	/* get free fd for reading */
	*readFd = thread_getFreeFd();
	if(*readFd < 0) {
		err = *readFd;
		goto errorRemNode;
	}
	/* open file for reading */
	readFile = vfs_openFile(t->tid,VFS_READ,pipeNodeNo,VFS_DEV_NO);
	if(readFile < 0) {
		err = readFile;
		goto errorRemNode;
	}
	/* assoc fd with file */
	err = thread_assocFd(*readFd,readFile);
	if(err < 0)
		goto errorCloseReadFile;

	/* get free fd for writing */
	*writeFd = thread_getFreeFd();
	if(*writeFd < 0)
		goto errorUnAssocReadFd;
	/* open file for writing */
	writeFile = vfs_openFile(t->tid,VFS_WRITE,pipeNodeNo,VFS_DEV_NO);
	if(writeFile < 0)
		goto errorUnAssocReadFd;
	/* assoc fd with file */
	err = thread_assocFd(*writeFd,writeFile);
	if(err < 0)
		goto errorCloseWriteFile;

	/* yay, we're done! :) */
	SYSC_RET1(stack,0);
	return;

	/* error-handling */
errorCloseWriteFile:
	vfs_closeFile(t->tid,writeFile);
errorUnAssocReadFd:
	thread_unassocFd(*readFd);
errorCloseReadFile:
	vfs_closeFile(t->tid,readFile);
errorRemNode:
	vfsn_removeNode(pipeNode);
	SYSC_ERROR(stack,err);
}

void sysc_tell(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u32 *pos = (u32*)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	tFileNo file;

	if(!paging_isRangeUserWritable((u32)pos,sizeof(u32)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	*pos = vfs_tell(t->tid,file);
	SYSC_RET1(stack,0);
}

void sysc_eof(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	bool eof;

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	eof = vfs_eof(t->tid,file);
	SYSC_RET1(stack,eof);
}

void sysc_seek(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	s32 offset = (s32)SYSC_ARG2(stack);
	u32 whence = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_seek(t->tid,file,offset,whence);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_read(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	s32 readBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	readBytes = vfs_readFile(t->tid,file,buffer,count);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);

	SYSC_RET1(stack,readBytes);
}

void sysc_write(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	s32 writtenBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserReadable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	writtenBytes = vfs_writeFile(t->tid,file,buffer,count);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);

	SYSC_RET1(stack,writtenBytes);
}

void sysc_ioctl(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u32 cmd = SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	if(data != NULL && size == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* TODO always writable ok? */
	if(data != NULL && !paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* perform io-control */
	res = vfs_ioctl(t->tid,file,cmd,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_send(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId id = (tMsgId)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	/* validate size and data */
	if(size <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserReadable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_sendMsg(t->tid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_receive(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId *id = (tMsgId*)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	s32 res;

	/* validate id and data */
	if(!paging_isRangeUserWritable((u32)id,sizeof(tMsgId)) ||
			!paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = thread_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_receiveMsg(t->tid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_dupFd(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tFD res;

	res = thread_dupFd(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_redirFd(sIntrptStackFrame *stack) {
	tFD src = (tFD)SYSC_ARG1(stack);
	tFD dst = (tFD)SYSC_ARG2(stack);
	s32 err;

	err = thread_redirFd(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);

	SYSC_RET1(stack,err);
}

void sysc_close(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sThread *t = thread_getRunning();

	/* unassoc fd */
	tFileNo fileNo = thread_unassocFd(fd);
	if(fileNo < 0)
		return;

	/* close file */
	vfs_closeFile(t->tid,fileNo);
}

void sysc_stat(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	tInodeNo nodeNo;
	u32 len;
	s32 res;

	if(!sysc_isStringReadable(path) || info == NULL ||
			!paging_isRangeUserWritable((u32)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	len = strlen(path);
	if(len == 0 || len >= MAX_PATH_LEN)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfsn_resolvePath(path,&nodeNo,NULL,VFS_READ);
	if(res == ERR_REAL_PATH) {
		sThread *t = thread_getRunning();
		res = vfsr_stat(t->tid,path,info);
	}
	else if(res == 0)
		res = vfsn_getNodeInfo(nodeNo,info);

	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

void sysc_sync(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	res = vfsr_sync(t->tid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_link(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *oldPath = (char*)SYSC_ARG1(stack);
	char *newPath = (char*)SYSC_ARG2(stack);
	if(!sysc_isStringReadable(oldPath) || !sysc_isStringReadable(newPath))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_link(t->tid,oldPath,newPath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unlink(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_unlink(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_mkdir(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_mkdir(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_rmdir(sIntrptStackFrame *stack) {
	s32 res;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_rmdir(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_mount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sThread *t = thread_getRunning();
	char *device = (char*)SYSC_ARG1(stack);
	char *path = (char*)SYSC_ARG2(stack);
	u16 type = (u16)SYSC_ARG3(stack);
	if(!sysc_isStringReadable(device) || !sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_mount(t->tid,device,path,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unmount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sThread *t = thread_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_unmount(t->tid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
