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
#include <sys/vfs/rw.h>
#include <sys/syscalls/driver.h>
#include <sys/syscalls.h>
#include <errors.h>

/* implementable functions */
#define DRV_ALL						(DRV_OPEN | DRV_READ | DRV_WRITE | DRV_CLOSE | DRV_TERM)

#define GW_NOBLOCK					1

void sysc_regDriver(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	u32 flags = SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tDrvId res;

	/* check flags */
	if((flags & ~DRV_ALL) != 0 && flags != DRV_FS)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(!sysc_isStringReadable(name))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	res = vfs_createDriver(p->pid,name,flags);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unregDriver(sIntrptStackFrame *stack) {
	tDrvId id = SYSC_ARG1(stack);
	sProc *p = proc_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* remove the driver */
	err = vfs_removeDriver(p->pid,id);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

void sysc_setDataReadable(sIntrptStackFrame *stack) {
	tDrvId id = SYSC_ARG1(stack);
	bool readable = (bool)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = vfs_setDataReadable(p->pid,id,readable);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

void sysc_getClientId(sIntrptStackFrame *stack) {
	tFD fd = (tFD)SYSC_ARG1(stack);
	tFileNo file;
	tInodeNo id;
	sProc *p = proc_getRunning();

	file = proc_fdToFile(fd);
	if(file < 0)
		SYSC_ERROR(stack,file);

	id = vfs_getClientId(p->pid,file);
	if(id < 0)
		SYSC_ERROR(stack,id);
	SYSC_RET1(stack,id);
}

void sysc_getClient(sIntrptStackFrame *stack) {
	tDrvId did = (tDrvId)SYSC_ARG1(stack);
	tInodeNo cid = (tInodeNo)SYSC_ARG2(stack);
	sProc *p = proc_getRunning();
	tFD fd;
	tFileNo file;
	s32 res;

	/* we need a file-desc */
	fd = proc_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open client */
	file = vfs_openClient(p->pid,did,cid);
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

void sysc_getWork(sIntrptStackFrame *stack) {
	tDrvId *ids = (tDrvId*)SYSC_ARG1(stack);
	u32 idCount = SYSC_ARG2(stack);
	tDrvId *drv = (tDrvId*)SYSC_ARG3(stack);
	tMsgId *id = (tMsgId*)SYSC_ARG4(stack);
	u8 *data = (u8*)SYSC_ARG5(stack);
	u32 size = SYSC_ARG6(stack);
	u8 flags = (u8)SYSC_ARG7(stack);
	sThread *t = thread_getRunning();
	tFileNo file;
	tFD fd;
	sVFSNode *cnode;
	tInodeNo client;
	s32 res;

	/* validate driver-ids */
	if(idCount <= 0 || idCount > 32 || ids == NULL ||
			!paging_isRangeUserReadable((u32)ids,idCount * sizeof(tDrvId)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* validate id and data */
	if(!paging_isRangeUserWritable((u32)id,sizeof(tMsgId)) ||
			!paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* check drv */
	if(drv != NULL && !paging_isRangeUserWritable((u32)drv,sizeof(tDrvId)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* open a client */
	while(1) {
		client = vfs_getClient(t->proc->pid,(tInodeNo*)ids,idCount);
		if(client != ERR_NO_CLIENT_WAITING)
			break;

		/* if we shouldn't block, stop here */
		if(flags & GW_NOBLOCK)
			SYSC_ERROR(stack,client);

		/* otherwise wait for a client (accept signals) */
		thread_wait(t->tid,NULL,EV_CLIENT);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	if(client < 0)
		SYSC_ERROR(stack,client);

	/* get fd for communication with the client */
	fd = proc_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open file */
	file = vfs_openFile(t->proc->pid,VFS_READ | VFS_WRITE,client,VFS_DEV_NO);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* assoc with fd */
	res = proc_assocFd(fd,file);
	if(res < 0) {
		vfs_closeFile(t->proc->pid,file);
		SYSC_ERROR(stack,res);
	}

	/* receive a message */
	cnode = vfsn_getNode(client);
	res = vfsrw_readDrvUse(t->proc->pid,file,cnode,id,data,size);
	if(res < 0) {
		proc_unassocFd(fd);
		vfs_closeFile(t->proc->pid,file);
		SYSC_ERROR(stack,res);
	}

	if(drv)
		*drv = NADDR_TO_VNNO(cnode->parent);
	SYSC_RET1(stack,fd);
}
