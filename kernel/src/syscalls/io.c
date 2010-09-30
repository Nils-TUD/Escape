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
#include <sys/mem/paging.h>
#include <sys/vfs/real.h>
#include <sys/vfs/driver.h>
#include <sys/task/thread.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls.h>
#include <errors.h>
#include <string.h>

void sysc_open(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	s32 pathLen;
	u16 flags;
	tFileNo file;
	tFD fd;
	s32 err;
	sProc *p = proc_getRunning();

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

	/* open the path */
	file = vfs_openPath(p->pid,flags,path);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* get free fd */
	fd = proc_getFreeFd();
	if(fd < 0) {
		vfs_closeFile(p->pid,file);
		SYSC_ERROR(stack,fd);
	}

	/* assoc fd with file */
	err = proc_assocFd(fd,file);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,fd);
}

void sysc_pipe(sIntrptStackFrame *stack) {
	tFD *readFd = (tFD*)SYSC_ARG1(stack);
	tFD *writeFd = (tFD*)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
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
	err = vfsn_resolvePath("/system/pipe",&nodeNo,&created,VFS_READ);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* create pipe */
	err = vfsn_createUse(p->pid,vfsn_getNode(nodeNo),&pipeNode);
	if(err < 0)
		SYSC_ERROR(stack,err);

	pipeNodeNo = vfsn_getNodeNo(pipeNode);
	/* get free fd for reading */
	*readFd = proc_getFreeFd();
	if(*readFd < 0) {
		err = *readFd;
		goto errorRemNode;
	}
	/* open file for reading */
	readFile = vfs_openFile(p->pid,VFS_READ,pipeNodeNo,VFS_DEV_NO);
	if(readFile < 0) {
		err = readFile;
		goto errorRemNode;
	}
	/* assoc fd with file */
	err = proc_assocFd(*readFd,readFile);
	if(err < 0)
		goto errorCloseReadFile;

	/* get free fd for writing */
	*writeFd = proc_getFreeFd();
	if(*writeFd < 0)
		goto errorUnAssocReadFd;
	/* open file for writing */
	writeFile = vfs_openFile(p->pid,VFS_WRITE,pipeNodeNo,VFS_DEV_NO);
	if(writeFile < 0)
		goto errorUnAssocReadFd;
	/* assoc fd with file */
	err = proc_assocFd(*writeFd,writeFile);
	if(err < 0)
		goto errorCloseWriteFile;

	/* yay, we're done! :) */
	SYSC_RET1(stack,0);
	return;

	/* error-handling */
errorCloseWriteFile:
	vfs_closeFile(p->pid,writeFile);
errorUnAssocReadFd:
	proc_unassocFd(*readFd);
errorCloseReadFile:
	vfs_closeFile(p->pid,readFile);
	/* vfs_closeFile() will already remove the node, so we can't do this again! */
	SYSC_ERROR(stack,err);
errorRemNode:
	vfsn_removeNode(pipeNode);
	SYSC_ERROR(stack,err);
}

void sysc_tell(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	s32 *pos = (s32*)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tFileNo file;

	if(!paging_isRangeUserWritable((u32)pos,sizeof(u32)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	*pos = vfs_tell(p->pid,file);
	SYSC_RET1(stack,0);
}

void sysc_eof(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	bool eof;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	eof = vfs_eof(p->pid,file);
	SYSC_RET1(stack,eof);
}

void sysc_seek(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	s32 offset = (s32)SYSC_ARG2(stack);
	u32 whence = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_seek(p->pid,file,offset,whence);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_read(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	s32 readBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserWritable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	readBytes = vfs_readFile(p->pid,file,buffer,count);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);

	SYSC_RET1(stack,readBytes);
}

void sysc_write(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	u8 *buffer = (u8*)SYSC_ARG2(stack);
	u32 count = SYSC_ARG3(stack);
	sProc *p = proc_getRunning();
	s32 writtenBytes;
	tFileNo file;

	/* validate count and buffer */
	if(count <= 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isRangeUserReadable((u32)buffer,count))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	writtenBytes = vfs_writeFile(p->pid,file,buffer,count);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);

	SYSC_RET1(stack,writtenBytes);
}

void sysc_hasMsg(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* perform io-control */
	res = vfs_hasMsg(p->pid,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_isterm(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	bool res;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_isterm(p->pid,file);
	SYSC_RET1(stack,res);
}

void sysc_send(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId id = (tMsgId)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	/* validate size and data */
	if(data != NULL && !paging_isRangeUserReadable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_sendMsg(p->pid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_receive(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tMsgId *id = (tMsgId*)SYSC_ARG2(stack);
	u8 *data = (u8*)SYSC_ARG3(stack);
	u32 size = SYSC_ARG4(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	/* validate id and data */
	if((id != NULL && !paging_isRangeUserWritable((u32)id,sizeof(tMsgId))) ||
			(data != NULL && !paging_isRangeUserWritable((u32)data,size)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_receiveMsg(p->pid,file,id,data,size);
	if(res < 0)
		SYSC_ERROR(stack,res);

	SYSC_RET1(stack,res);
}

void sysc_dupFd(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tFD res;

	res = proc_dupFd(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_redirFd(sIntrptStackFrame *stack) {
	tFD src = (tFD)SYSC_ARG1(stack);
	tFD dst = (tFD)SYSC_ARG2(stack);
	s32 err;

	err = proc_redirFd(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

void sysc_close(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sProc *p = proc_getRunning();

	/* unassoc fd */
	tFileNo fileNo = proc_unassocFd(fd);
	if(fileNo < 0)
		return;

	/* close file */
	vfs_closeFile(p->pid,fileNo);
}

void sysc_stat(sIntrptStackFrame *stack) {
	char *path = (char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	u32 len;
	s32 res;

	if(!sysc_isStringReadable(path) || info == NULL ||
			!paging_isRangeUserWritable((u32)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	len = strlen(path);
	if(len == 0 || len >= MAX_PATH_LEN)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_stat(p->pid,path,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

void sysc_fstat(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tFileNo file;
	s32 res;

	if(info == NULL || !paging_isRangeUserWritable((u32)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);
	/* get info */
	res = vfs_fstat(p->pid,file,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

void sysc_sync(sIntrptStackFrame *stack) {
	s32 res;
	sProc *p = proc_getRunning();
	res = vfsr_sync(p->pid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_link(sIntrptStackFrame *stack) {
	s32 res;
	sProc *p = proc_getRunning();
	char *oldPath = (char*)SYSC_ARG1(stack);
	char *newPath = (char*)SYSC_ARG2(stack);
	if(!sysc_isStringReadable(oldPath) || !sysc_isStringReadable(newPath))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_link(p->pid,oldPath,newPath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unlink(sIntrptStackFrame *stack) {
	s32 res;
	sProc *p = proc_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_unlink(p->pid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_mkdir(sIntrptStackFrame *stack) {
	s32 res;
	sProc *p = proc_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_mkdir(p->pid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_rmdir(sIntrptStackFrame *stack) {
	s32 res;
	sProc *p = proc_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_rmdir(p->pid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_mount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sProc *p = proc_getRunning();
	char *device = (char*)SYSC_ARG1(stack);
	char *path = (char*)SYSC_ARG2(stack);
	u16 type = (u16)SYSC_ARG3(stack);
	if(!sysc_isStringReadable(device) || !sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_mount(p->pid,device,path,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unmount(sIntrptStackFrame *stack) {
	s32 res;
	tInodeNo ino;
	sProc *p = proc_getRunning();
	char *path = (char*)SYSC_ARG1(stack);
	if(!sysc_isStringReadable(path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfsn_resolvePath(path,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfsr_unmount(p->pid,path);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
