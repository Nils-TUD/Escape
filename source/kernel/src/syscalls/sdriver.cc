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

#include <sys/driver.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/signals.h>
#include <task/thread.h>
#include <vfs/channel.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <errno.h>
#include <string.h>
#include <syscalls.h>

int Syscalls::createdev(Thread *t,IntrptStackFrame *stack) {
	char filename[MAX_PATH_LEN + 1];
	int fd = (int)SYSC_ARG1(stack);
	const char *name = (const char*)SYSC_ARG2(stack);
	mode_t mode = SYSC_ARG3(stack);
	uint type = SYSC_ARG4(stack) & ((1 << BITS_DEV_TYPE) - 1);
	uint ops = SYSC_ARG4(stack) >> BITS_DEV_TYPE;
	Proc *p = t->getProc();
	if(EXPECT_FALSE(!copyPath(filename,sizeof(filename),name)))
		SYSC_ERROR(stack,-EFAULT);

	/* check type and ops */
	if(EXPECT_FALSE(type != DEV_TYPE_BLOCK && type != DEV_TYPE_CHAR &&
			type != DEV_TYPE_FILE && type != DEV_TYPE_SERVICE && type != DEV_TYPE_FS))
		SYSC_ERROR(stack,-EINVAL);
	if(EXPECT_FALSE((ops & ~(DEV_OPEN | DEV_READ | DEV_WRITE | DEV_CLOSE | DEV_SHFILE |
			DEV_CANCEL | DEV_CANCELSIG | DEV_CREATSIBL | DEV_SIZE)) != 0))
		SYSC_ERROR(stack,-EINVAL);
	/* DEV_CLOSE is mandatory */
	if(EXPECT_FALSE(~ops & DEV_CLOSE))
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	OpenFile *dir = FileDesc::request(p,fd);
	if(EXPECT_FALSE(dir == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* create device and open it */
	OpenFile *file;
	int res = dir->createdev(p->getPid(),name,mode,type,ops,&file);
	FileDesc::release(dir);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	/* assoc fd with it */
	int nfd = FileDesc::assoc(p,file);
	if(EXPECT_FALSE(nfd < 0))
		SYSC_ERROR(stack,nfd);
	SYSC_RET1(stack,nfd);
}

int Syscalls::bindto(Thread *t,IntrptStackFrame *stack) {
	int fd = SYSC_ARG1(stack);
	tid_t tid = SYSC_ARG2(stack);
	Proc *p = t->getProc();
	OpenFile *file;

	/* get file */
	file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* perform operation */
	int res = file->bindto(tid);
	FileDesc::release(file);
	if(res < 0)
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
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
	if(EXPECT_FALSE(!client))
		SYSC_ERROR(stack,-EBADF);

	/* receive a message */
	res = client->receiveMsg(p->getPid(),&mid,data,size,VFS_SIGNALS);
	FileDesc::release(client);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	*id = mid;
	SYSC_RET1(stack,clifd);
}
