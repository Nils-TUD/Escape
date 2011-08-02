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
#include <sys/mem/paging.h>
#include <sys/vfs/real.h>
#include <sys/vfs/pipe.h>
#include <sys/vfs/driver.h>
#include <sys/vfs/node.h>
#include <sys/task/thread.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls.h>
#include <errors.h>
#include <string.h>

int sysc_open(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint flags = (uint)SYSC_ARG2(stack);
	file_t file;
	int fd;
	const sProc *p = proc_getRunning();
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* check flags */
	flags &= VFS_WRITE | VFS_READ | VFS_MSGS | VFS_CREATE | VFS_TRUNCATE | VFS_APPEND | VFS_NOBLOCK;
	if((flags & (VFS_READ | VFS_WRITE | VFS_MSGS)) == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* open the path */
	file = vfs_openPath(p->pid,flags,abspath);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* assoc fd with file */
	fd = proc_assocFd(file);
	if(fd < 0) {
		vfs_closeFile(p->pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int sysc_fcntl(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uint cmd = SYSC_ARG2(stack);
	int arg = (int)SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	int res;

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_fcntl(p->pid,file,cmd,arg);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_pipe(sIntrptStackFrame *stack) {
	int *readFd = (int*)SYSC_ARG1(stack);
	int *writeFd = (int*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	bool created;
	sVFSNode *pipeNode;
	inode_t nodeNo,pipeNodeNo;
	file_t readFile,writeFile;
	int kreadFd,kwriteFd;
	int err;

	/* make sure that the pointers point to userspace */
	if(!paging_isInUserSpace((uintptr_t)readFd,sizeof(int)) ||
			!paging_isInUserSpace((uintptr_t)writeFd,sizeof(int)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* resolve pipe-path */
	err = vfs_node_resolvePath("/system/pipe",&nodeNo,&created,VFS_READ);
	if(err < 0)
		SYSC_ERROR(stack,err);

	/* create pipe */
	pipeNode = vfs_pipe_create(p->pid,vfs_node_get(nodeNo));
	if(pipeNode == NULL)
		SYSC_ERROR(stack,ERR_NOT_ENOUGH_MEM);

	pipeNodeNo = vfs_node_getNo(pipeNode);
	/* open file for reading */
	readFile = vfs_openFile(p->pid,VFS_READ,pipeNodeNo,VFS_DEV_NO);
	if(readFile < 0) {
		err = readFile;
		goto errorRemNode;
	}
	/* assoc fd with file */
	kreadFd = proc_assocFd(readFile);
	if(kreadFd < 0) {
		err = kreadFd;
		goto errorCloseReadFile;
	}

	/* open file for writing */
	writeFile = vfs_openFile(p->pid,VFS_WRITE,pipeNodeNo,VFS_DEV_NO);
	if(writeFile < 0)
		goto errorUnAssocReadFd;
	/* assoc fd with file */
	kwriteFd = proc_assocFd(writeFile);
	if(kwriteFd < 0) {
		err = kwriteFd;
		goto errorCloseWriteFile;
	}

	/* now copy the fds to userspace; this might cause a pagefault and might even cause a kill */
	/* this is no problem because we've associated the fds, so that all resources will be free'd */
	*readFd = kreadFd;
	*writeFd = kwriteFd;

	/* yay, we're done! :) */
	SYSC_RET1(stack,0);

	/* error-handling */
errorCloseWriteFile:
	vfs_closeFile(p->pid,writeFile);
errorUnAssocReadFd:
	proc_unassocFd(kreadFd);
errorCloseReadFile:
	vfs_closeFile(p->pid,readFile);
	/* vfs_closeFile() will already remove the node, so we can't do this again! */
	SYSC_ERROR(stack,err);
errorRemNode:
	vfs_node_destroy(pipeNode);
	SYSC_ERROR(stack,err);
}

int sysc_stat(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_stat(p->pid,abspath,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_fstat(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
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

int sysc_chmod(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_chmod(p->pid,abspath,mode);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_chown(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uid_t uid = (uid_t)SYSC_ARG2(stack);
	gid_t gid = (gid_t)SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_chown(p->pid,abspath,uid,gid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_tell(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t *pos = (off_t*)SYSC_ARG2(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	if(!paging_isInUserSpace((uintptr_t)pos,sizeof(off_t)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* get file */
	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	*pos = vfs_tell(p->pid,file);
	SYSC_RET1(stack,0);
}

int sysc_seek(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t offset = (off_t)SYSC_ARG2(stack);
	uint whence = SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	off_t res;

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

int sysc_read(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	ssize_t readBytes;
	file_t file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
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

int sysc_write(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const void *buffer = (const void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	const sProc *p = proc_getRunning();
	ssize_t writtenBytes;
	file_t file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
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

int sysc_send(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	const void *data = (const void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
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

int sysc_receive(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	const sProc *p = proc_getRunning();
	file_t file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
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

int sysc_dupFd(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int res;

	res = proc_dupFd(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_redirFd(sIntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);
	int err = proc_redirFd(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

int sysc_close(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const sProc *p = proc_getRunning();

	/* unassoc fd */
	file_t fileNo = proc_unassocFd(fd);
	if(fileNo < 0)
		SYSC_ERROR(stack,fileNo);

	/* close file */
	vfs_closeFile(p->pid,fileNo);
	SYSC_RET1(stack,0);
}

int sysc_sync(sIntrptStackFrame *stack) {
	int res;
	const sProc *p = proc_getRunning();
	res = vfs_real_sync(p->pid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_link(sIntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	int res;
	const sProc *p = proc_getRunning();
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(!sysc_absolutize_path(oldabs,sizeof(oldabs),oldPath))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!sysc_absolutize_path(newabs,sizeof(newabs),newPath))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_link(p->pid,oldabs,newabs);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unlink(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	const sProc *p = proc_getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_unlink(p->pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mkdir(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	const sProc *p = proc_getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_mkdir(p->pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_rmdir(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	const sProc *p = proc_getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_rmdir(p->pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mount(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	char absdev[MAX_PATH_LEN + 1];
	int res;
	inode_t ino;
	const sProc *p = proc_getRunning();
	const char *device = (const char*)SYSC_ARG1(stack);
	const char *path = (const char*)SYSC_ARG2(stack);
	uint type = (uint)SYSC_ARG3(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!sysc_absolutize_path(absdev,sizeof(absdev),device))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfs_node_resolvePath(abspath,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfs_real_mount(p->pid,absdev,abspath,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unmount(sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	inode_t ino;
	const sProc *p = proc_getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(vfs_node_resolvePath(abspath,&ino,NULL,VFS_READ) != ERR_REAL_PATH)
		SYSC_ERROR(stack,ERR_MOUNT_VIRT_PATH);

	res = vfs_real_unmount(p->pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
