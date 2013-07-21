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
#include <sys/mem/virtmem.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/filedesc.h>
#include <sys/syscalls.h>
#include <esc/messages.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int Syscalls::open(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint flags = (uint)SYSC_ARG2(stack);
	sFile *file;
	int res,fd;
	pid_t pid = t->getProc()->getPid();
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	/* check flags */
	flags &= VFS_USER_FLAGS;
	if((flags & (VFS_READ | VFS_WRITE | VFS_MSGS)) == 0)
		SYSC_ERROR(stack,-EINVAL);

	/* open the path */
	res = vfs_openPath(pid,flags,abspath,&file);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with file */
	fd = FileDesc::assoc(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int Syscalls::fcntl(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uint cmd = SYSC_ARG2(stack);
	int arg = (int)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	int res;

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	res = vfs_fcntl(pid,file,cmd,arg);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::pipe(Thread *t,sIntrptStackFrame *stack) {
	int *readFd = (int*)SYSC_ARG1(stack);
	int *writeFd = (int*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *readFile,*writeFile;
	int kreadFd,kwriteFd;
	int res;

	/* make sure that the pointers point to userspace */
	if(!paging_isInUserSpace((uintptr_t)readFd,sizeof(int)) ||
			!paging_isInUserSpace((uintptr_t)writeFd,sizeof(int)))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_openPipe(pid,&readFile,&writeFile);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with read-file */
	kreadFd = FileDesc::assoc(readFile);
	if(kreadFd < 0) {
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kreadFd);
	}

	/* assoc fd with write-file */
	kwriteFd = FileDesc::assoc(writeFile);
	if(kwriteFd < 0) {
		FileDesc::unassoc(kreadFd);
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kwriteFd);
	}

	/* now copy the fds to userspace; this may fail, but we have file-descriptors for the files,
	 * so the resources will be free'd in any case. */
	*readFd = kreadFd;
	*writeFd = kwriteFd;

	/* yay, we're done! :) */
	SYSC_RET1(stack,res);
}

int Syscalls::stat(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_stat(pid,abspath,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::fstat(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);
	/* get info */
	res = vfs_fstat(pid,file,info);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::chmod(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chmod(pid,abspath,mode);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::chown(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uid_t uid = (uid_t)SYSC_ARG2(stack);
	gid_t gid = (gid_t)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	int res;
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chown(pid,abspath,uid,gid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::tell(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t *pos = (off_t*)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	if(!paging_isInUserSpace((uintptr_t)pos,sizeof(off_t)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* this may fail, but we're requested the file, so it will be released on our termination */
	*pos = vfs_tell(pid,file);
	FileDesc::release(file);
	SYSC_RET1(stack,0);
}

int Syscalls::seek(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t offset = (off_t)SYSC_ARG2(stack);
	uint whence = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	off_t res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	res = vfs_seek(pid,file,offset,whence);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::read(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	ssize_t readBytes;
	sFile *file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* read */
	readBytes = vfs_readFile(pid,file,buffer,count);
	FileDesc::release(file);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);
	SYSC_RET1(stack,readBytes);
}

int Syscalls::write(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const void *buffer = (const void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	ssize_t writtenBytes;
	sFile *file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* read */
	writtenBytes = vfs_writeFile(pid,file,buffer,count);
	FileDesc::release(file);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);
	SYSC_RET1(stack,writtenBytes);
}

int Syscalls::send(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	const void *data = (const void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);
	/* can't be sent by user-programs */
	if(IS_DEVICE_MSG(id))
		SYSC_ERROR(stack,-EPERM);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* send msg */
	res = vfs_sendMsg(pid,file,id,data,size,NULL,0);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::receive(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* send msg */
	res = vfs_receiveMsg(pid,file,id,data,size,false);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::dup(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int res;

	res = FileDesc::dup(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::redirect(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);
	int err = FileDesc::redirect(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

int Syscalls::close(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	pid_t pid = t->getProc()->getPid();

	sFile *file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* close file */
	FileDesc::unassoc(fd);
	if(!vfs_closeFile(pid,file))
		FileDesc::release(file);
	else
		Thread::remFileUsage(file);
	SYSC_RET1(stack,0);
}

int Syscalls::sync(Thread *t,sIntrptStackFrame *stack) {
	int res;
	pid_t pid = t->getProc()->getPid();
	res = vfs_sync(pid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::link(Thread *t,sIntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(!absolutizePath(oldabs,sizeof(oldabs),oldPath))
		SYSC_ERROR(stack,-EFAULT);
	if(!absolutizePath(newabs,sizeof(newabs),newPath))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_link(pid,oldabs,newabs);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::unlink(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unlink(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::mkdir(A_UNUSED Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = Proc::getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mkdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::rmdir(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_rmdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::mount(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	char absdev[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *device = (const char*)SYSC_ARG1(stack);
	const char *path = (const char*)SYSC_ARG2(stack);
	uint type = (uint)SYSC_ARG3(stack);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);
	if(!absolutizePath(absdev,sizeof(absdev),device))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mount(pid,absdev,abspath,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::unmount(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unmount(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
