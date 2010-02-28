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

#include <common.h>
#include <mem/paging.h>
#include <task/thread.h>
#include <task/signals.h>
#include <vfs/rw.h>
#include <syscalls/service.h>
#include <syscalls.h>
#include <errors.h>

/* service-types, as defined in libc/include/esc/service.h */
#define SERV_DEFAULT				1
#define SERV_FS						2
#define SERV_DRIVER					4
#define SERV_ALL					(SERV_DEFAULT | SERV_FS | SERV_DRIVER)

#define GW_NOBLOCK					1

void sysc_regService(sIntrptStackFrame *stack) {
	const char *name = (const char*)SYSC_ARG1(stack);
	u32 type = (u32)SYSC_ARG2(stack);
	u32 vtype;
	sThread *t = thread_getRunning();
	tServ res;

	/* check type */
	if((type & SERV_ALL) == 0 || (type & ~SERV_ALL) != 0)
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* convert and check type */
	vtype = 0;
	if(type & SERV_FS)
		vtype |= MODE_SERVICE_FS;
	else if(type & SERV_DRIVER)
		vtype |= MODE_SERVICE_DRIVER;

	res = vfs_createService(t->tid,name,vtype);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

void sysc_unregService(sIntrptStackFrame *stack) {
	tServ id = SYSC_ARG1(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* remove the service */
	err = vfs_removeService(t->tid,id);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

void sysc_setDataReadable(sIntrptStackFrame *stack) {
	tServ id = SYSC_ARG1(stack);
	bool readable = (bool)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	s32 err;

	/* check node-number */
	if(!vfsn_isValidNodeNo(id))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	err = vfs_setDataReadable(t->tid,id,readable);
	if(err < 0)
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,0);
}

void sysc_getClientThread(sIntrptStackFrame *stack) {
	tServ id = (tServ)SYSC_ARG1(stack);
	tTid tid = (tPid)SYSC_ARG2(stack);
	sThread *t = thread_getRunning();
	tFD fd;
	tFileNo file;
	s32 res;

	if(thread_getById(tid) == NULL)
		SYSC_ERROR(stack,ERR_INVALID_TID);

	/* we need a file-desc */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open client */
	file = vfs_openClientThread(t->tid,id,tid);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* associate fd with file */
	res = thread_assocFd(fd,file);
	if(res < 0) {
		/* we have already opened the file */
		vfs_closeFile(t->tid,file);
		SYSC_ERROR(stack,res);
	}

	SYSC_RET1(stack,fd);
}

void sysc_getWork(sIntrptStackFrame *stack) {
	tServ *ids = (tServ*)SYSC_ARG1(stack);
	u32 idCount = SYSC_ARG2(stack);
	tServ *serv = (tServ*)SYSC_ARG3(stack);
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

	/* validate service-ids */
	if(idCount <= 0 || idCount > 32 || ids == NULL ||
			!paging_isRangeUserReadable((u32)ids,idCount * sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* validate id and data */
	if(!paging_isRangeUserWritable((u32)id,sizeof(tMsgId)) ||
			!paging_isRangeUserWritable((u32)data,size))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	/* check serv */
	if(serv != NULL && !paging_isRangeUserWritable((u32)serv,sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* open a client */
	while(1) {
		client = vfs_getClient(t->tid,(tInodeNo*)ids,idCount);
		if(client != ERR_NO_CLIENT_WAITING)
			break;

		/* if we shouldn't block, stop here */
		if(flags & GW_NOBLOCK)
			SYSC_ERROR(stack,client);

		/* otherwise wait for a client (accept signals) */
		thread_wait(t->tid,EV_CLIENT);
		thread_switch();
		if(sig_hasSignalFor(t->tid))
			SYSC_ERROR(stack,ERR_INTERRUPTED);
	}
	if(client < 0)
		SYSC_ERROR(stack,client);

	/* get fd for communication with the client */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open file */
	file = vfs_openFile(t->tid,VFS_READ | VFS_WRITE,client,VFS_DEV_NO);
	if(file < 0)
		SYSC_ERROR(stack,file);

	/* assoc with fd */
	res = thread_assocFd(fd,file);
	if(res < 0) {
		vfs_closeFile(t->tid,file);
		SYSC_ERROR(stack,res);
	}

	/* receive a message */
	cnode = vfsn_getNode(client);
	res = vfsrw_readServUse(t->tid,file,cnode,id,data,size);
	if(res < 0) {
		thread_unassocFd(fd);
		vfs_closeFile(t->tid,file);
		SYSC_ERROR(stack,res);
	}

	if(serv)
		*serv = NADDR_TO_VNNO(cnode->parent);
	SYSC_RET1(stack,fd);
}
