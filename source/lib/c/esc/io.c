/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/common.h>
#include <esc/io.h>
#include <esc/messages.h>
#include <esc/mem.h>
#include <esc/dir.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

int open(const char *path,uint flags) {
	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_OPEN,(ulong)abspath(apath,sizeof(apath),path),flags,FILE_DEF_MODE);
}

int create(const char *path,uint flags,mode_t mode) {
	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_OPEN,(ulong)abspath(apath,sizeof(apath),path),flags | IO_CREATE,mode);
}

int stat(const char *path,sFileInfo *info) {
	char apath[MAX_PATH_LEN];
	return syscall2(SYSCALL_STAT,(ulong)abspath(apath,sizeof(apath),path),(ulong)info);
}

int chmod(const char *path,mode_t mode) {
	char apath[MAX_PATH_LEN];
	return syscall2(SYSCALL_CHMOD,(ulong)abspath(apath,sizeof(apath),path),mode);
}

int chown(const char *path,uid_t uid,gid_t gid) {
	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_CHOWN,(ulong)abspath(apath,sizeof(apath),path),uid,gid);
}

int link(const char *oldPath,const char *newPath) {
	char apath1[MAX_PATH_LEN];
	char apath2[MAX_PATH_LEN];
	oldPath = abspath(apath1,sizeof(apath1),oldPath);
	newPath = abspath(apath2,sizeof(apath2),newPath);
	return syscall2(SYSCALL_LINK,(ulong)oldPath,(ulong)newPath);
}

int unlink(const char *path) {
	char apath[MAX_PATH_LEN];
	return syscall1(SYSCALL_UNLINK,(ulong)abspath(apath,sizeof(apath),path));
}

int mkdir(const char *path) {
	char apath[MAX_PATH_LEN];
	return syscall1(SYSCALL_MKDIR,(ulong)abspath(apath,sizeof(apath),path));
}

int rmdir(const char *path) {
	char apath[MAX_PATH_LEN];
	return syscall1(SYSCALL_RMDIR,(ulong)abspath(apath,sizeof(apath),path));
}

int mount(int fd,const char *path) {
	char apath[MAX_PATH_LEN];
	return syscall2(SYSCALL_MOUNT,fd,(ulong)abspath(apath,sizeof(apath),path));
}

int unmount(const char *path) {
	char apath[MAX_PATH_LEN];
	return syscall1(SYSCALL_UNMOUNT,(ulong)abspath(apath,sizeof(apath),path));
}

int pipe(int *readFd,int *writeFd) {
	/* the permissions are read-write for both. we ensure that the first is for reading only and
	 * the second for writing only in the pipe-driver. */
	int fd = open("/dev/pipe",IO_READ | IO_WRITE);
	if(fd < 0)
		return fd;
	*writeFd = fd;
	*readFd = creatsibl(fd,0);
	if(*readFd < 0) {
		close(fd);
		return *readFd;
	}
	return 0;
}

int sharebuf(int dev,size_t size,void **mem,ulong *name,int flags) {
	/* create shm file */
	*mem = NULL;
	int fd = pshm_create(IO_READ | IO_WRITE,0666,name);
	if(fd < 0)
		return fd;

	/* mmap it */
	void *addr = mmap(NULL,size,0,PROT_READ | PROT_WRITE,MAP_SHARED | flags,fd,0);
	if(!addr) {
		int res = errno;
		pshm_unlink(*name);
		close(fd);
		return res;
	}

	/* share it with device; if it doesn't work, we don't care here */
	int res = sharefile(dev,addr);
	*mem = addr;
	close(fd);
	return res;
}

void *joinbuf(const char *path,size_t size,int flags) {
	int fd = open(path,IO_READ | IO_WRITE);
	if(fd < 0)
		return NULL;
	void *res = mmap(NULL,size,0,PROT_READ | PROT_WRITE,MAP_SHARED | flags,fd,0);
	/* keep errno of mmap */
	int error = errno;
	close(fd);
	errno = error;
	return res;
}

void destroybuf(void *mem,ulong name) {
	pshm_unlink(name);
	munmap(mem);
}

bool isfile(const char *path) {
	sFileInfo info;
	if(stat(path,&info) < 0)
		return false;
	return S_ISREG(info.mode);
}

bool isdir(const char *path) {
	sFileInfo info;
	if(stat(path,&info) < 0)
		return false;
	return S_ISDIR(info.mode);
}
