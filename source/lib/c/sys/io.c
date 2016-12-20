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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static const char *splitPath(char *tmp,char *apath,const char *path,char **name) {
	/* copy it to the stack first, because abspath might return the third argument, which has to
	 * be writable, because dirfile needs to change it */
	strnzcpy(tmp,path,MAX_PATH_LEN);
	char *fullpath = abspath(apath,MAX_PATH_LEN,tmp);
	return dirfile(fullpath,name);
}

int open(const char *path,uint flags,...) {
	va_list ap;
	va_start(ap, flags);
	mode_t mode = va_arg(ap, int);
	va_end(ap);

	char apath[MAX_PATH_LEN];
	return syscall3(SYSCALL_OPEN,(ulong)abspath(apath,sizeof(apath),path),flags,mode);
}

int truncate(const char *path,off_t length) {
	int fd = open(path,O_WRONLY);
	if(fd < 0)
		return fd;
	int res = ftruncate(fd,length);
	close(fd);
	errno = res;
	return res;
}

int link(const char *oldPath,const char *newPath) {
	int target = open(oldPath,O_NOCHAN);
	if(target < 0)
		return target;

	char *name, apath[MAX_PATH_LEN];
	char tmp[MAX_PATH_LEN];
	const char *dirPath = splitPath(tmp,apath,newPath,&name);

	int dir = open(dirPath,O_WRITE);
	if(dir < 0) {
		close(target);
		return dir;
	}
	int res = flink(target,dir,name);
	close(dir);
	close(target);
	errno = res;
	return res;
}

int flink(int target,int dir,const char *name) {
	return syscall3(SYSCALL_LINK,target,dir,(ulong)name);
}

int unlink(const char *path) {
	char *name, apath[MAX_PATH_LEN];
	char tmp[MAX_PATH_LEN];
	const char *dirPath = splitPath(tmp,apath,path,&name);

	int fd = open(dirPath,O_WRITE);
	if(fd < 0)
		return fd;
	int res = funlink(fd,name);
	close(fd);
	errno = res;
	return res;
}

int funlink(int dir,const char *name) {
	return syscall2(SYSCALL_UNLINK,dir,(ulong)name);
}

int rename(const char *oldPath,const char *newPath) {
	char *oldName, oldAPath[MAX_PATH_LEN];
	char oldTmp[MAX_PATH_LEN];
	const char *oldDirPath = splitPath(oldTmp,oldAPath,oldPath,&oldName);

	int oldDir = open(oldDirPath,O_WRITE);
	if(oldDir < 0)
		return oldDir;

	char *newName, newAPath[MAX_PATH_LEN];
	char newTmp[MAX_PATH_LEN];
	const char *newDirPath = splitPath(newTmp,newAPath,newPath,&newName);

	int newDir = open(newDirPath,O_WRITE);
	if(newDir < 0) {
		close(oldDir);
		return newDir;
	}
	int res = frename(oldDir,oldName,newDir,newName);
	close(newDir);
	close(oldDir);
	errno = res;
	return res;
}

int frename(int olddir,const char *oldname,int newdir,const char *newname) {
	return syscall4(SYSCALL_RENAME,olddir,(ulong)oldname,newdir,(ulong)newname);
}

int mkdir(const char *path,mode_t mode) {
	char *name, apath[MAX_PATH_LEN];
	char tmp[MAX_PATH_LEN];
	const char *dirPath = splitPath(tmp,apath,path,&name);

	int fd = open(dirPath,O_WRITE);
	if(fd < 0)
		return fd;
	int res = fmkdir(fd,name,mode);
	close(fd);
	errno = res;
	return res;
}

int fmkdir(int fd,const char *name,mode_t mode) {
	return syscall3(SYSCALL_MKDIR,fd,(ulong)name,mode);
}

int rmdir(const char *path) {
	char *name, apath[MAX_PATH_LEN];
	char tmp[MAX_PATH_LEN];
	const char *dirPath = splitPath(tmp,apath,path,&name);

	int fd = open(dirPath,O_WRITE);
	if(fd < 0)
		return fd;
	int res = frmdir(fd,name);
	close(fd);
	errno = res;
	return res;
}

int frmdir(int fd,const char *name) {
	return syscall2(SYSCALL_RMDIR,fd,(ulong)name);
}

int pipe(int *readFd,int *writeFd) {
	int fd = open("/dev/pipe",O_WRONLY);
	if(fd < 0)
		return fd;
	*writeFd = fd;
	*readFd = obtain(fd,0);
	if(*readFd < 0) {
		close(fd);
		errno = *readFd;
		return *readFd;
	}
	return 0;
}

int createbuf(size_t size,void **mem,int flags) {
	/* create shm file */
	*mem = NULL;
	int fd = opentmp();
	if(fd < 0)
		return fd;

	int res;
	if((res = ftruncate(fd,size)) < 0)
		return res;

	/* mmap it */
	void *addr = mmap(NULL,size,size,PROT_READ | PROT_WRITE,MAP_SHARED | flags,fd,0);
	if(!addr) {
		res = errno;
		close(fd);
		errno = res;
		return res;
	}

	*mem = addr;
	return fd;
}

int sharebuf(int dev,size_t size,void **mem,int flags) {
	int fd = createbuf(size,mem,flags);
	if(fd < 0)
		return fd;

	/* share it with device; if it doesn't work, we don't care here */
	int res = delegate(dev,fd,O_RDWR,DEL_ARG_SHFILE);
	if(res < 0)
		return res;
	return fd;
}

void destroybuf(void *mem,int fd) {
	munmap(mem);
	close(fd);
}
