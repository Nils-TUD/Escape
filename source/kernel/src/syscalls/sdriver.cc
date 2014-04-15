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
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/filedesc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/openfile.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/syscalls.h>
#include <errno.h>
#include <string.h>

int Syscalls::createdev(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = SYSC_ARG2(stack);
	uint type = SYSC_ARG3(stack);
	uint ops = SYSC_ARG4(stack);
	pid_t pid = t->getProc()->getPid();
	if(EXPECT_FALSE(!absolutizePath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	/* check type and ops */
	if(EXPECT_FALSE(type != DEV_TYPE_BLOCK && type != DEV_TYPE_CHAR &&
			type != DEV_TYPE_FILE && type != DEV_TYPE_SERVICE))
		SYSC_ERROR(stack,-EINVAL);
	if(EXPECT_FALSE((ops & ~(DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE | DEV_CANCEL)) != 0))
		SYSC_ERROR(stack,-EINVAL);
	/* DEV_CLOSE is mandatory */
	if(EXPECT_FALSE(~ops & DEV_CLOSE))
		SYSC_ERROR(stack,-EINVAL);

	/* create device and open it */
	OpenFile *file;
	int res = VFS::createdev(pid,abspath,mode,type,ops,&file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	/* assoc fd with it */
	int fd = FileDesc::assoc(t->getProc(),file);
	if(EXPECT_FALSE(fd < 0))
		SYSC_ERROR(stack,fd);
	SYSC_RET1(stack,fd);
}

int Syscalls::getwork(Thread *t,IntrptStackFrame *stack) {
	int fd = SYSC_ARG1(stack) >> 2;
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	uint flags = SYSC_ARG1(stack) & 0x3;
	Proc *p = t->getProc();
	OpenFile *file;
	msgid_t mid = *id;

	/* validate pointers */
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)id,sizeof(msgid_t))))
		SYSC_ERROR(stack,-EFAULT);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)data,size)))
		SYSC_ERROR(stack,-EFAULT);

	/* translate to files */
	file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* open a client */
	int clifd;
	ssize_t res = OpenFile::getWork(file,&clifd,flags);

	/* release files */
	FileDesc::release(file);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	OpenFile *client = FileDesc::request(p,clifd);
	if(!client)
		SYSC_ERROR(stack,-EBADF);

	/* receive a message */
	res = client->receiveMsg(p->getPid(),&mid,data,size,VFS_SIGNALS);
	FileDesc::release(client);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	*id = mid;
	SYSC_RET1(stack,clifd);
}
