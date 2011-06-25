/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

/* implementable functions */
#define MAX_GETWORK_DRIVERS			16
#define DRV_ALL						(DRV_OPEN | DRV_READ | DRV_WRITE | DRV_CLOSE | DRV_TERM)

#define GW_NOBLOCK					1

int sysc_regDriver(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	uint flags = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	int fd;
	file_t res;

	/* check flags */
	if((flags & ~DRV_ALL) != 0 && flags != DRV_FS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* create driver and open it */
	res = vfs_createDriver(p->pid,name,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with it */
	fd = proc_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);
	res = proc_assocFd(fd,res);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,fd);
}

int sysc_getClientId(sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	file_t file;
	inode_t id;
	sProc *p = proc_getRunning();

	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	id = vfs_getClientId(p->pid,file);
	if(id < 0)
		SYSC_ERROR(stack,id);
	SYSC_RET1(stack,id);
}

int sysc_getClient(sIntrptStackFrame *stack) {
	int drvFd = (int)SYSC_ARG1(stack);
	inode_t cid = (inode_t)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	int fd;
	file_t file,drvFile;
	int res;

	/* get driver-file */
	drvFile = proc_fdToFile(drvFd);
	if(drvFile < 0)
		SYSC_ERROR(stack,drvFile);

	/* we need a file-desc */
	fd = proc_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open client */
	file = vfs_openClient(p->pid,drvFile,cid);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* associate fd with file */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		/* we have already opened the file */
		vfs_closeFile(p->pid,file);
		SYSC_ERROR(stack,res);
	}
	SYSC_RET1(stack,fd);
}

int sysc_getWork(sIntrptStackFrame *stack) {
	file_t files[MAX_GETWORK_DRIVERS];
	sWaitObject waits[MAX_GETWORK_DRIVERS];
	int *fds = (int*)SYSC_ARG1(stack);
	size_t fdCount = SYSC_ARG2(stack);
	int *drv = (int*)SYSC_ARG3(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG4(stack);
	void *data = (void*)SYSC_ARG5(stack);
	size_t size = SYSC_ARG6(stack);
	uint flags = (uint)SYSC_ARG7(stack);
	sThread *t = thread_getRunning();
	file_t file;
	inode_t clientNo;
	int fd;
	size_t i,index;
	ssize_t res;

	/* validate driver-ids */
	if(fdCount <= 0 || fdCount > MAX_GETWORK_DRIVERS || fds == NULL ||
			!paging_isRangeUserReadable((uintptr_t)fds,fdCount * sizeof(int)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* validate id and data */
	if(!paging_isRangeUserWritable((uintptr_t)id,sizeof(msgid_t)) ||
			!paging_isRangeUserWritable((uintptr_t)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* check drv */
	if(drv != NULL && !paging_isRangeUserWritable((uintptr_t)drv,sizeof(int)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* translate to files */
	for(i = 0; i < fdCount; i++) {
		files[i] = proc_fdToFile(fds[i]);
		if(files[i] < 0)
			SYSC_ERROR(stack,files[i]);
		waits[i].events = EV_CLIENT;
		waits[i].object = (evobj_t)vfs_getVNode(files[i]);
	}

	/* open a client */
	while(1) {
		clientNo = vfs_getClient(t->proc->pid,files,fdCount,&index);
		if(clientNo != ERR_NO_CLIENT_WAITING)
			break;

		/* if we shouldn't block, stop here */
		if(flags & GW_NOBLOCK)
			SYSC_ERROR(stack,clientNo);

		/* otherwise wait for a client (accept signals) */
		ev_waitObjects(t->tid,waits,fdCount);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	if(clientNo < 0)
		SYSC_ERROR(stack,clientNo);

	/* get fd for communication with the client */
	fd = proc_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open file */
	file = vfs_openFile(t->proc->pid,VFS_READ | VFS_WRITE | VFS_DRIVER,clientNo,VFS_DEV_NO);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* receive a message */
	res = vfs_receiveMsg(t->proc->pid,file,id,data,size);
	if(res < 0) {
		vfs_closeFile(t->proc->pid,file);
		SYSC_ERROR(stack,res);
	}

	/* assoc with fd */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		vfs_closeFile(t->proc->pid,file);
		SYSC_ERROR(stack,res);
	}

	if(drv)
		*drv = fds[index];
	SYSC_RET1(stack,fd);
}
