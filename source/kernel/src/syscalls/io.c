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
#include <sys/mem/vmm.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/task/thread.h>
#include <sys/syscalls/io.h>
#include <sys/syscalls.h>
#include <esc/messages.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

int sysc_open(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint flags = (uint)SYSC_ARG2(stack);
	file_t file;
	int fd;
	pid_t pid = t->proc->pid;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	/* check flags */
	flags &= VFS_WRITE | VFS_READ | VFS_MSGS | VFS_CREATE | VFS_TRUNCATE | VFS_APPEND | VFS_NOBLOCK;
	if((flags & (VFS_READ | VFS_WRITE | VFS_MSGS)) == 0)
		SYSC_ERROR(stack,-EINVAL);

	/* open the path */
	file = vfs_openPath(pid,flags,abspath);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* assoc fd with file */
	fd = proc_assocFd(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int sysc_fcntl(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uint cmd = SYSC_ARG2(stack);
	int arg = (int)SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	int res;

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_fcntl(pid,file,cmd,arg);
	proc_relFile(t,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_pipe(sThread *t,sIntrptStackFrame *stack) {
	int *readFd = (int*)SYSC_ARG1(stack);
	int *writeFd = (int*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	file_t readFile,writeFile;
	int kreadFd,kwriteFd;
	int res;
	sProc *p;

	/* make sure that the pointers point to userspace */
	if(!paging_isInUserSpace((uintptr_t)readFd,sizeof(int)) ||
			!paging_isInUserSpace((uintptr_t)writeFd,sizeof(int)))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_openPipe(pid,&readFile,&writeFile);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with read-file */
	kreadFd = proc_assocFd(readFile);
	if(kreadFd < 0) {
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kreadFd);
	}

	/* assoc fd with write-file */
	kwriteFd = proc_assocFd(writeFile);
	if(kwriteFd < 0) {
		proc_unassocFd(kreadFd);
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,kwriteFd);
	}

	/* now copy the fds to userspace; ensure that this is safe */
	p = proc_request(pid,PLOCK_REGIONS);
	if(!vmm_makeCopySafe(p,readFd,sizeof(int)) || !vmm_makeCopySafe(p,writeFd,sizeof(int))) {
		proc_release(p,PLOCK_REGIONS);
		proc_unassocFd(kwriteFd);
		proc_unassocFd(kreadFd);
		vfs_closeFile(pid,readFile);
		vfs_closeFile(pid,writeFile);
		SYSC_ERROR(stack,-EFAULT);
	}
	*readFd = kreadFd;
	*writeFd = kwriteFd;
	proc_release(p,PLOCK_REGIONS);

	/* yay, we're done! :) */
	SYSC_RET1(stack,res);
}

int sysc_stat(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_stat(pid,abspath,info);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_fstat(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	sFileInfo *info = (sFileInfo*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	int res;
	if(!paging_isInUserSpace((uintptr_t)info,sizeof(sFileInfo)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);
	/* get info */
	res = vfs_fstat(pid,file,info);
	proc_relFile(t,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_chmod(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chmod(pid,abspath,mode);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_chown(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uid_t uid = (uid_t)SYSC_ARG2(stack);
	gid_t gid = (gid_t)SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	int res;
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_chown(pid,abspath,uid,gid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int sysc_tell(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t *pos = (off_t*)SYSC_ARG2(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	sProc *p;
	if(!paging_isInUserSpace((uintptr_t)pos,sizeof(off_t)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	p = proc_request(pid,PLOCK_REGIONS);
	if(!vmm_makeCopySafe(p,pos,sizeof(off_t))) {
		proc_release(p,PLOCK_REGIONS);
		proc_relFile(t,file);
		SYSC_ERROR(stack,-EFAULT);
	}
	*pos = vfs_tell(pid,file);
	proc_release(p,PLOCK_REGIONS);
	proc_relFile(t,file);
	SYSC_RET1(stack,0);
}

int sysc_seek(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t offset = (off_t)SYSC_ARG2(stack);
	uint whence = SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	off_t res;

	if(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	res = vfs_seek(pid,file,offset,whence);
	proc_relFile(t,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_read(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	ssize_t readBytes;
	file_t file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	readBytes = vfs_readFile(pid,file,buffer,count);
	proc_relFile(t,file);
	if(readBytes < 0)
		SYSC_ERROR(stack,readBytes);
	SYSC_RET1(stack,readBytes);
}

int sysc_write(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const void *buffer = (const void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	pid_t pid = t->proc->pid;
	ssize_t writtenBytes;
	file_t file;

	/* validate count and buffer */
	if(count == 0)
		SYSC_ERROR(stack,-EINVAL);
	if(!paging_isInUserSpace((uintptr_t)buffer,count))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* read */
	writtenBytes = vfs_writeFile(pid,file,buffer,count);
	proc_relFile(t,file);
	if(writtenBytes < 0)
		SYSC_ERROR(stack,writtenBytes);
	SYSC_RET1(stack,writtenBytes);
}

int sysc_send(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	const void *data = (const void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);
	/* can't be sent by user-programs */
	if(IS_DEVICE_MSG(id))
		SYSC_ERROR(stack,-EPERM);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_sendMsg(pid,file,id,data,size);
	proc_relFile(t,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_receive(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	pid_t pid = t->proc->pid;
	file_t file;
	ssize_t res;
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* send msg */
	res = vfs_receiveMsg(pid,file,id,data,size);
	proc_relFile(t,file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_dupFd(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int res;

	res = proc_dupFd(fd);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_redirFd(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);
	int err = proc_redirFd(src,dst);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

int sysc_close(sThread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	pid_t pid = t->proc->pid;

	file_t file = proc_reqFile(t,fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* close file */
	proc_unassocFd(fd);
	if(!vfs_closeFile(pid,file))
		proc_relFile(t,file);
	else
		thread_remFileUsage(t,file);
	SYSC_RET1(stack,0);
}

int sysc_sync(sThread *t,sIntrptStackFrame *stack) {
	int res;
	pid_t pid = t->proc->pid;
	res = vfs_sync(pid);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_link(sThread *t,sIntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->proc->pid;
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(!sysc_absolutize_path(oldabs,sizeof(oldabs),oldPath))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(newabs,sizeof(newabs),newPath))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_link(pid,oldabs,newabs);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unlink(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->proc->pid;
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unlink(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mkdir(A_UNUSED sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = proc_getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mkdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_rmdir(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->proc->pid;
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_rmdir(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_mount(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	char absdev[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->proc->pid;
	const char *device = (const char*)SYSC_ARG1(stack);
	const char *path = (const char*)SYSC_ARG2(stack);
	uint type = (uint)SYSC_ARG3(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);
	if(!sysc_absolutize_path(absdev,sizeof(absdev),device))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_mount(pid,absdev,abspath,type);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int sysc_unmount(sThread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	int res;
	pid_t pid = t->proc->pid;
	const char *path = (const char*)SYSC_ARG1(stack);
	if(!sysc_absolutize_path(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	res = vfs_unmount(pid,abspath);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
