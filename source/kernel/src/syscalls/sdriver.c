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
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/syscalls/driver.h>
#include <sys/syscalls.h>
#include <errors.h>
#include <string.h>

/* implementable functions */
#define MAX_GETWORK_DRIVERS			16
#define DRV_ALL						(DRV_OPEN | DRV_READ | DRV_WRITE | DRV_CLOSE)

#define GW_NOBLOCK					1

int sysc_regDriver(sIntrptStackFrame *stack) {
	char nameCpy[MAX_PATH_LEN + 1];
	const char *name = (const char*)SYSC_ARG1(stack);
	uint flags = SYSC_ARG2(stack);
	pid_t pid = proc_getRunning();
	int fd;
	file_t res;

	/* check flags */
	if((flags & ~DRV_ALL) != 0 && flags != DRV_FS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	strncpy(nameCpy,name,sizeof(nameCpy));
	nameCpy[sizeof(nameCpy) - 1] = '\0';

	/* create driver and open it */
	res = vfs_createDriver(pid,nameCpy,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with it */
	fd = proc_assocFd(res);
	if(fd < 0)
		SYSC_ERROR(stack,fd);
	SYSC_RET1(stack,fd);
}

int sysc_getClientId(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	file_t file;
	inode_t id;
	pid_t pid = proc_getRunning();

	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	id = vfs_getClientId(pid,file);
	if(id < 0)
		SYSC_ERROR(stack,id);
	SYSC_RET1(stack,id);
}

int sysc_getClient(sIntrptStackFrame *stack) {
	int drvFd = (int)SYSC_ARG1(stack);
	inode_t cid = (inode_t)SYSC_ARG2(stack);
	pid_t pid = proc_getRunning();
	int fd;
	file_t file,drvFile;

	/* get driver-file */
	drvFile = proc_fdToFile(drvFd);
	if(drvFile < 0)
		SYSC_ERROR(stack,drvFile);

	/* open client */
	file = vfs_openClient(pid,drvFile,cid);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* associate fd with file */
	fd = proc_assocFd(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int sysc_getWork(sIntrptStackFrame *stack) {
	file_t files[MAX_GETWORK_DRIVERS];
	sWaitObject waits[MAX_GETWORK_DRIVERS];
	const int *fds = (const int*)SYSC_ARG1(stack);
	size_t fdCount = SYSC_ARG2(stack);
	int *drv = (int*)SYSC_ARG3(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG4(stack);
	void *data = (void*)SYSC_ARG5(stack);
	size_t size = SYSC_ARG6(stack);
	uint flags = (uint)SYSC_ARG7(stack);
	sThread *t = thread_getRunning();
	pid_t pid = t->proc->pid;
	file_t file;
	inode_t clientNo;
	int fd;
	size_t i,index;
	ssize_t res;
	bool inited = false;

	/* validate pointers */
	if(fdCount == 0 || fdCount > MAX_GETWORK_DRIVERS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)drv,sizeof(int)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)id,sizeof(msgid_t)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!paging_isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* translate to files */
	for(i = 0; i < fdCount; i++) {
		files[i] = proc_fdToFile(fds[i]);
		if(files[i] < 0)
			SYSC_ERROR(stack,files[i]);
	}

	/* open a client */
	while(1) {
		clientNo = vfs_getClient(pid,files,fdCount,&index);
		if(clientNo != ERR_NO_CLIENT_WAITING)
			break;

		/* if we shouldn't block, stop here */
		if(flags & GW_NOBLOCK)
			SYSC_ERROR(stack,clientNo);

		if(!inited) {
			for(i = 0; i < fdCount; i++) {
				waits[i].events = EV_CLIENT;
				waits[i].object = (evobj_t)vfs_getVNode(files[i]);
			}
			inited = true;
		}

		/* otherwise wait for a client (accept signals) */
		ev_waitObjects(t,waits,fdCount);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	if(clientNo < 0)
		SYSC_ERROR(stack,clientNo);

	/* open file */
	file = vfs_openFile(pid,VFS_MSGS | VFS_DRIVER,clientNo,VFS_DEV_NO);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* receive a message */
	res = vfs_receiveMsg(pid,file,id,data,size);
	if(res < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,res);
	}

	/* assoc with fd */
	fd = proc_assocFd(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}

	if(drv)
		*drv = fds[index];
	SYSC_RET1(stack,fd);
}
