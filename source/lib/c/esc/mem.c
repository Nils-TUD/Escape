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
#include <esc/atomic.h>
#include <esc/proc.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* prefill the space for getpid() with '/'. they will simply be ignored by the kernel */
#define PSHM_PATH	"/system/processes/////////////shm/"

static long shmcnt = 0;

/* just a convenience for the user which sets errno if the return-value is zero (not enough mem) */
void *chgsize(ssize_t count) {
	size_t addr = syscall1(SYSCALL_CHGSIZE,count);
	if(addr == 0)
		errno = -ENOMEM;
	return (void*)addr;
}

void *mmapphys(uintptr_t *phys,size_t count,size_t align) {
	intptr_t addr = syscall3(SYSCALL_MAPPHYS,(ulong)phys,count,align);
	/* FIXME workaround until we have TLS */
	if(addr >= -200 && addr < 0)
		return NULL;
	return (void*)addr;
}

void *mmap(void *addr,size_t length,size_t loadLength,int prot,int flags,int fd,off_t offset) {
	intptr_t res = syscall7(SYSCALL_MMAP,(ulong)addr,length,loadLength,prot,flags,fd,offset);
	/* FIXME workaround until we have TLS */
	if(res >= -200 && res < 0)
		return NULL;
	return (void*)res;
}

static const char *pshm_buildpath(char *path,ulong name) {
	char *pidpos = path + SSTRLEN("/system/processes/");
	size_t len = itoa(pidpos,12,getpid());
	pidpos[len] = '/';
	itoa(path + SSTRLEN(PSHM_PATH),12,name);
	return path;
}

int pshm_create(int oflag,mode_t mode,ulong *name) {
	char path[MAX_PATH_LEN] = PSHM_PATH;
	*name = atomic_add(&shmcnt,+1);
	return create(pshm_buildpath(path,*name),oflag | IO_CREATE,mode);
}

int pshm_unlink(ulong name) {
	char path[MAX_PATH_LEN] = PSHM_PATH;
	return unlink(pshm_buildpath(path,name));
}

int shm_open(const char *name,int oflag,mode_t mode) {
	int fd;
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/shm/%s",name);
	if(oflag & IO_CREATE)
		fd = create(path,oflag,mode);
	else
		fd = open(path,oflag);
	return fd;
}

int shm_rename(const char *old,const char *newName) {
	char pathOld[MAX_PATH_LEN];
	char pathNew[MAX_PATH_LEN];
	snprintf(pathOld,sizeof(pathOld),"/system/shm/%s",old);
	snprintf(pathNew,sizeof(pathNew),"/system/shm/%s",newName);
	return rename(pathOld,pathNew);
}

int shm_unlink(const char *name) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/shm/%s",name);
	return unlink(path);
}
