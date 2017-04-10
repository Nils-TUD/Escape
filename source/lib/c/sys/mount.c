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
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscalls.h>
#include <dirent.h>

int mount(int ms,int fs,const char *path) {
	/* root is a special case; we cannot create it before, which is why it doesn't exist before
	 * mounting something there */
	if(path[0] == '/' && path[1] == '\0')
		return syscall4(SYSCALL_MOUNT,ms,fs,(ulong)"/",0);
	return remount(ms,fs,path,0);
}

int remount(int ms,int dir,const char *path,uint perm) {
	char apath[MAX_PATH_LEN];
	ssize_t res = canonpath(apath,sizeof(apath),path);
	if(res < 0)
		return res;
	return syscall4(SYSCALL_MOUNT,ms,dir,(ulong)apath,perm);
}

int unmount(int ms,const char *path) {
	char apath[MAX_PATH_LEN];
	ssize_t res = canonpath(apath,sizeof(apath),path);
	if(res < 0)
		return res;
	return syscall2(SYSCALL_UNMOUNT,ms,(ulong)apath);
}
