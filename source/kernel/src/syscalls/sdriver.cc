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
#include <sys/mem/virtmem.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/task/filedesc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/syscalls.h>
#include <errno.h>
#include <string.h>

int Syscalls::createdev(Thread *t,sIntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint type = SYSC_ARG2(stack);
	uint ops = SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	int res,fd;
	sFile *file;
	if(!absolutizePath(abspath,sizeof(abspath),path))
		SYSC_ERROR(stack,-EFAULT);

	/* check type and ops */
	if(type != DEV_TYPE_BLOCK && type != DEV_TYPE_CHAR && type != DEV_TYPE_FS &&
			type != DEV_TYPE_FILE && type != DEV_TYPE_SERVICE)
		SYSC_ERROR(stack,-EINVAL);
	if(type != DEV_TYPE_FS && (ops & ~(DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE)) != 0)
		SYSC_ERROR(stack,-EINVAL);

	/* create device and open it */
	res = vfs_createdev(pid,abspath,type,ops,&file);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* assoc fd with it */
	fd = FileDesc::assoc(file);
	if(fd < 0)
		SYSC_ERROR(stack,fd);
	SYSC_RET1(stack,fd);
}

int Syscalls::getclientid(Thread *t,sIntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	sFile *file;
	inode_t id;
	pid_t pid = t->getProc()->getPid();

	file = FileDesc::request(fd);
	if(file == NULL)
		SYSC_ERROR(stack,-EBADF);

	id = vfs_getClientId(pid,file);
	FileDesc::release(file);
	if(id < 0)
		SYSC_ERROR(stack,id);
	SYSC_RET1(stack,id);
}

int Syscalls::getclient(Thread *t,sIntrptStackFrame *stack) {
	int drvFd = (int)SYSC_ARG1(stack);
	inode_t cid = (inode_t)SYSC_ARG2(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file,*drvFile;
	int res,fd;

	/* get file */
	drvFile = FileDesc::request(drvFd);
	if(drvFile == NULL)
		SYSC_ERROR(stack,-EBADF);

	/* open client */
	res = vfs_openClient(pid,drvFile,cid,&file);
	FileDesc::release(drvFile);
	if(res < 0)
		SYSC_ERROR(stack,res);

	/* associate fd with file */
	fd = FileDesc::assoc(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int Syscalls::getwork(Thread *t,sIntrptStackFrame *stack) {
	sFile *files[MAX_GETWORK_DEVICES];
	const int *fds = (const int*)SYSC_ARG1(stack);
	size_t fdCount = SYSC_ARG2(stack);
	int *drv = (int*)SYSC_ARG3(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG4(stack);
	void *data = (void*)SYSC_ARG5(stack);
	size_t size = SYSC_ARG6(stack);
	uint flags = (uint)SYSC_ARG7(stack);
	pid_t pid = t->getProc()->getPid();
	sFile *file;
	inode_t clientNo;
	int fd;
	size_t i,index;
	ssize_t res;

	/* validate pointers */
	if(fdCount == 0 || fdCount > MAX_GETWORK_DEVICES)
		SYSC_ERROR(stack,-EINVAL);
	if(!PageDir::isInUserSpace((uintptr_t)drv,sizeof(int)))
		SYSC_ERROR(stack,-EFAULT);
	if(!PageDir::isInUserSpace((uintptr_t)id,sizeof(msgid_t)))
		SYSC_ERROR(stack,-EFAULT);
	if(!PageDir::isInUserSpace((uintptr_t)data,size))
		SYSC_ERROR(stack,-EFAULT);

	/* translate to files */
	for(i = 0; i < fdCount; i++) {
		files[i] = FileDesc::request(fds[i]);
		if(files[i] == NULL) {
			for(; i > 0; i--)
				FileDesc::release(files[i - 1]);
			SYSC_ERROR(stack,-EBADF);
		}
	}

	/* open a client */
	clientNo = vfs_getClient(files,fdCount,&index,flags);

	/* release files */
	for(i = 0; i < fdCount; i++)
		FileDesc::release(files[i]);

	if(clientNo < 0)
		SYSC_ERROR(stack,clientNo);

	/* open file */
	res = vfs_openFile(pid,VFS_MSGS | VFS_DEVICE,clientNo,VFS_DEV_NO,&file);
	if(res < 0) {
		/* we have to set the channel unused again; otherwise its ignored for ever */
		vfs_chan_setUsed(vfs_node_get(clientNo),false);
		SYSC_ERROR(stack,res);
	}

	/* receive a message */
	res = vfs_receiveMsg(pid,file,id,data,size,false);
	if(res < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,res);
	}

	/* assoc with fd */
	fd = FileDesc::assoc(file);
	if(fd < 0) {
		vfs_closeFile(pid,file);
		SYSC_ERROR(stack,fd);
	}

	if(drv)
		*drv = fds[index];
	SYSC_RET1(stack,fd);
}
