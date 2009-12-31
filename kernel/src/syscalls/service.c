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
#include <syscalls/service.h>
#include <syscalls.h>
#include <errors.h>

/* service-types, as defined in libc/include/esc/service.h */
#define SERV_DEFAULT				1
#define SERV_FS						2
#define SERV_DRIVER					4
#define SERV_ALL					(SERV_DEFAULT | SERV_FS | SERV_DRIVER)

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

void sysc_getClient(sIntrptStackFrame *stack) {
	tServ *ids = (tServ*)SYSC_ARG1(stack);
	u32 count = SYSC_ARG2(stack);
	tServ *client = (tServ*)SYSC_ARG3(stack);
	sThread *t = thread_getRunning();
	tFD fd;
	s32 res;
	tFileNo file;

	/* check arguments. limit count a little bit to prevent overflow */
	if(count <= 0 || count > 32 || ids == NULL ||
			!paging_isRangeUserReadable((u32)ids,count * sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);
	if(client == NULL || !paging_isRangeUserWritable((u32)client,sizeof(tServ)))
		SYSC_ERROR(stack,ERR_INVALID_ARGS);

	/* we need a file-desc */
	fd = thread_getFreeFd();
	if(fd < 0)
		SYSC_ERROR(stack,fd);

	/* open a client */
	file = vfs_openClient(t->tid,(tInodeNo*)ids,count,(tInodeNo*)client);
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
