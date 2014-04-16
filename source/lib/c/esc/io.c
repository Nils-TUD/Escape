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
#include <errno.h>
#include <string.h>
#include <stdarg.h>

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
	close(fd);
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
