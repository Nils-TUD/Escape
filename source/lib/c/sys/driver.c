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
#include <sys/driver.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <dirent.h>
#include <string.h>

int createdev(const char *path,mode_t mode,uint type,uint ops) {
	char *name, apath[MAX_PATH_LEN];
	char tmp[MAX_PATH_LEN];
	/* copy it to the stack first, because abspath might return the third argument, which has to
	 * be writable, because dirfile needs to change it */
	strnzcpy(tmp,path,MAX_PATH_LEN);
	const char *dirPath = dirfile(abspath(apath,MAX_PATH_LEN,tmp),&name);

	int fd = open(dirPath,O_NOCHAN);
	if(fd < 0)
		return fd;
	int res = fcreatedev(fd,name,mode,type,ops);
	close(fd);
	errno = res;
	return res;
}

int fcreatedev(int dir,const char *name,mode_t mode,uint type,uint ops) {
	return syscall4(SYSCALL_CRTDEV,dir,(ulong)name,mode,type | (ops << BITS_DEV_TYPE));
}
