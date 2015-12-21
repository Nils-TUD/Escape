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
#include <sys/stat.h>
#include <sys/io.h>

int stat(const char *path,struct stat *info) {
	int fd = open(path,O_NOCHAN);
	if(fd < 0)
		return fd;
	int res = fstat(fd,info);
	close(fd);
	errno = res;
	return res;
}

int fstat(int fd,struct stat *info) {
	return syscall2(SYSCALL_FSTAT,fd,(ulong)info);
}

off_t filesize(int fd) {
	struct stat info;
	int res = fstat(fd,&info);
	if(res < 0)
		return res;
	return info.st_size;
}

int chmod(const char *path,mode_t mode) {
	int fd = open(path,O_NOCHAN);
	if(fd < 0)
		return fd;
	int res = fchmod(fd,mode);
	close(fd);
	errno = res;
	return res;
}

int fchmod(int fd,mode_t mode) {
	return syscall2(SYSCALL_CHMOD,fd,mode);
}

int chown(const char *path,uid_t uid,gid_t gid) {
	int fd = open(path,O_NOCHAN);
	if(fd < 0)
		return fd;
	int res = fchown(fd,uid,gid);
	close(fd);
	errno = res;
	return res;
}

int fchown(int fd,uid_t uid,gid_t gid) {
	return syscall3(SYSCALL_CHOWN,fd,uid,gid);
}

bool isfile(const char *path) {
	struct stat info;
	if(stat(path,&info) < 0)
		return false;
	return S_ISREG(info.st_mode);
}

bool isdir(const char *path) {
	struct stat info;
	if(stat(path,&info) < 0)
		return false;
	return S_ISDIR(info.st_mode);
}

bool isblock(const char *path) {
	struct stat info;
	if(stat(path,&info) < 0)
		return false;
	return S_ISREG(info.st_mode) || S_ISBLK(info.st_mode);
}
