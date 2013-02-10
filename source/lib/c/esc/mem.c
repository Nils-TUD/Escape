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

#include <esc/common.h>
#include <esc/mem.h>
#include <esc/fsinterface.h>
#include <stdio.h>
#include <errno.h>

/* the assembler-routine */
extern ssize_t _chgsize(ssize_t count);
extern intptr_t _regaddphys(uintptr_t *phys,size_t count,size_t align);
extern intptr_t _mmap(void *addr,size_t length,size_t loadLength,int prot,int flags,int fd,off_t offset);

/* just a convenience for the user which sets errno if the return-value is zero (not enough mem) */
void *chgsize(ssize_t count) {
	size_t addr = _chgsize(count);
	if(addr == 0)
		errno = -ENOMEM;
	return (void*)addr;
}

void *regaddphys(uintptr_t *phys,size_t count,size_t align) {
	intptr_t addr = _regaddphys(phys,count,align);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *mmap(void *addr,size_t length,size_t loadLength,int prot,int flags,int fd,off_t offset) {
	intptr_t res = _mmap(addr,length,loadLength,prot,flags,fd,offset);
	/* FIXME workaround until we have TLS */
	if(res >= -200 && res < 0)
		return NULL;
	return (void*)res;
}

int shm_open(const char *name,int oflag,mode_t mode) {
	int fd;
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/shm/%s",name);
	fd = open(path,oflag);
	/* TODO race-condition... */
	if(fd >= 0 && (oflag & IO_CREATE)) {
		int res = chmod(path,mode);
		if(res < 0) {
			close(fd);
			return res;
		}
	}
	return fd;
}

int shm_unlink(const char *name) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/shm/%s",name);
	return unlink(path);
}
