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

#include <mem/pagedir.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <sys/messages.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <string.h>
#include <syscalls.h>
#include <utime.h>

int Syscalls::open(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	const char *path = (const char*)SYSC_ARG1(stack);
	uint flags = (uint)SYSC_ARG2(stack);
	mode_t mode = (mode_t)SYSC_ARG3(stack);
	pid_t pid = t->getProc()->getPid();
	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	/* check flags */
	flags &= VFS_USER_FLAGS;
	if(EXPECT_FALSE((flags & (VFS_READ | VFS_WRITE | VFS_MSGS | VFS_NOCHAN)) == 0))
		SYSC_ERROR(stack,-EINVAL);

	/* open the path */
	OpenFile *file;
	int res = VFS::openPath(pid,flags,mode,abspath,&file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	/* assoc fd with file */
	int fd = FileDesc::assoc(t->getProc(),file);
	if(EXPECT_FALSE(fd < 0)) {
		file->close(pid);
		SYSC_ERROR(stack,fd);
	}
	SYSC_RET1(stack,fd);
}

int Syscalls::fcntl(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uint cmd = SYSC_ARG2(stack);
	int arg = (int)SYSC_ARG3(stack);
	Proc *p = t->getProc();

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->fcntl(p->getPid(),cmd,arg);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::fstat(Thread *t,IntrptStackFrame *stack) {
	struct stat kinfo;
	int fd = (int)SYSC_ARG1(stack);
	struct stat *info = (struct stat*)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(!PageDir::isInUserSpace((uintptr_t)info,sizeof(struct stat)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);
	/* get info */
	int res = file->fstat(p->getPid(),&kinfo);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	UserAccess::write(info,&kinfo,sizeof(kinfo));
	SYSC_RET1(stack,0);
}

int Syscalls::chmod(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->chmod(p->getPid(),mode);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::chown(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	uid_t uid = (uid_t)SYSC_ARG2(stack);
	gid_t gid = (gid_t)SYSC_ARG3(stack);
	Proc *p = t->getProc();

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->chown(p->getPid(),uid,gid);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::utime(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	struct utimbuf *utimes = (struct utimbuf*)SYSC_ARG2(stack);
	Proc *p = t->getProc();
	pid_t pid = p->getPid();
	struct utimbuf kutimes;

	if(EXPECT_FALSE(utimes != NULL && !PageDir::isInUserSpace((uintptr_t)utimes,sizeof(*utimes))))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	if(utimes)
		memcpy(&kutimes,utimes,sizeof(*utimes));
	else
		kutimes.actime = kutimes.modtime = Timer::getTime();

	int res = file->utime(pid,&kutimes);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::truncate(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t length = (off_t)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->truncate(p->getPid(),length);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,0);
}

int Syscalls::tell(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t *pos = (off_t*)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)pos,sizeof(off_t))))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* this may fail, but we're requested the file, so it will be released on our termination */
	*pos = file->tell(p->getPid());
	FileDesc::release(file);
	SYSC_RET1(stack,0);
}

int Syscalls::seek(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	off_t offset = (off_t)SYSC_ARG2(stack);
	uint whence = SYSC_ARG3(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END))
		SYSC_ERROR(stack,-EINVAL);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	off_t res = file->seek(p->getPid(),offset,whence);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::read(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	void *buffer = (void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	Proc *p = t->getProc();

	/* validate count and buffer */
	if(EXPECT_FALSE(count == 0))
		SYSC_ERROR(stack,-EINVAL);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)buffer,count)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* read */
	ssize_t readBytes = file->read(p->getPid(),buffer,count);
	FileDesc::release(file);
	if(EXPECT_FALSE(readBytes < 0))
		SYSC_ERROR(stack,readBytes);
	SYSC_RET1(stack,readBytes);
}

int Syscalls::write(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	const void *buffer = (const void*)SYSC_ARG2(stack);
	size_t count = SYSC_ARG3(stack);
	Proc *p = t->getProc();

	/* validate count and buffer */
	if(EXPECT_FALSE(count == 0))
		SYSC_ERROR(stack,-EINVAL);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)buffer,count)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* read */
	ssize_t writtenBytes = file->write(p->getPid(),buffer,count);
	FileDesc::release(file);
	if(EXPECT_FALSE(writtenBytes < 0))
		SYSC_ERROR(stack,writtenBytes);
	SYSC_RET1(stack,writtenBytes);
}

int Syscalls::send(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	const void *data = (const void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)data,size)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* can only be sent by drivers */
	if(EXPECT_FALSE(!file->isDevice() && IS_DEVICE_MSG(id & 0xFFFF))) {
		FileDesc::release(file);
		SYSC_ERROR(stack,-EPERM);
	}

	/* send msg */
	ssize_t res = file->sendMsg(p->getPid(),id,data,size,NULL,0);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::receive(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	Proc *p = t->getProc();
	/* prevent a pagefault during the operation */
	msgid_t mid = id != NULL ? *id : 0;

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)data,size)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* send msg */
	ssize_t res = file->receiveMsg(p->getPid(),&mid,data,size,VFS_SIGNALS);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	if(id)
		*id = mid;
	SYSC_RET1(stack,res);
}

int Syscalls::sendrecv(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t *id = (msgid_t*)SYSC_ARG2(stack);
	void *data = (void*)SYSC_ARG3(stack);
	size_t size = SYSC_ARG4(stack);
	Proc *p = t->getProc();
	msgid_t mid = *id;

	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)id,sizeof(id))))
		SYSC_ERROR(stack,-EFAULT);
	if(EXPECT_FALSE(!PageDir::isInUserSpace((uintptr_t)data,size)))
		SYSC_ERROR(stack,-EFAULT);

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* can only be sent by drivers */
	if(EXPECT_FALSE(!file->isDevice() && IS_DEVICE_MSG(mid & 0xFFFF))) {
		FileDesc::release(file);
		SYSC_ERROR(stack,-EPERM);
	}

	/* send msg */
	ssize_t res = file->sendMsg(p->getPid(),mid,data,size,NULL,0);
	if(EXPECT_FALSE(res < 0)) {
		FileDesc::release(file);
		SYSC_ERROR(stack,res);
	}

	/* receive response */
	mid = res;
	res = file->receiveMsg(p->getPid(),&mid,data,size,VFS_SIGNALS);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	*id = mid;
	SYSC_RET1(stack,res);
}

int Syscalls::cancel(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	msgid_t id = (msgid_t)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	/* get file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* share file */
	int res = file->cancel(p->getPid(),id);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::sharefile(Thread *t,IntrptStackFrame *stack) {
	char tmppath[MAX_PATH_LEN];
	int dev = (int)SYSC_ARG1(stack);
	void *mem = (void*)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	/* get file path */
	ssize_t size = p->getVM()->getShareInfo(reinterpret_cast<uintptr_t>(mem),tmppath,sizeof(tmppath));
	if(EXPECT_FALSE(size < 0))
		SYSC_ERROR(stack,size);

	/* get device file */
	OpenFile *file = FileDesc::request(p,dev);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* share file */
	int res = file->sharefile(p->getPid(),tmppath,mem,size);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::creatsibl(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	int arg = (int)SYSC_ARG2(stack);
	Proc *p = t->getProc();
	OpenFile *sibl;

	/* get channel file */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* create sibling */
	int res = VFS::creatsibl(p->getPid(),file,arg,&sibl);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);

	/* give it a file descriptor */
	int nfd = FileDesc::assoc(p,sibl);
	if(nfd < 0) {
		sibl->close(p->getPid());
		SYSC_ERROR(stack,res);
	}
	SYSC_RET1(stack,nfd);
}

int Syscalls::dup(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);

	int res = FileDesc::dup(fd);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::redirect(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);

	int err = FileDesc::redirect(src,dst);
	if(EXPECT_FALSE(err < 0))
		SYSC_ERROR(stack,err);
	SYSC_RET1(stack,err);
}

int Syscalls::close(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	Proc *p = t->getProc();

	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* close file */
	FileDesc::unassoc(p,fd);
	if(EXPECT_FALSE(!file->close(p->getPid())))
		FileDesc::release(file);
	SYSC_RET1(stack,0);
}

int Syscalls::syncfs(Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);
	Proc *p = t->getProc();

	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->syncfs(p->getPid());
	FileDesc::release(file);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::link(Thread *t,IntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	pid_t pid = t->getProc()->getPid();
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(EXPECT_FALSE(!copyPath(oldabs,sizeof(oldabs),oldPath)))
		SYSC_ERROR(stack,-EFAULT);
	if(EXPECT_FALSE(!copyPath(newabs,sizeof(newabs),newPath)))
		SYSC_ERROR(stack,-EFAULT);

	int res = VFS::link(pid,oldabs,newabs);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::unlink(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	int res = VFS::unlink(pid,abspath);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::rename(Thread *t,IntrptStackFrame *stack) {
	char oldabs[MAX_PATH_LEN + 1];
	char newabs[MAX_PATH_LEN + 1];
	pid_t pid = t->getProc()->getPid();
	const char *oldPath = (const char*)SYSC_ARG1(stack);
	const char *newPath = (const char*)SYSC_ARG2(stack);
	if(EXPECT_FALSE(!copyPath(oldabs,sizeof(oldabs),oldPath)))
		SYSC_ERROR(stack,-EFAULT);
	if(EXPECT_FALSE(!copyPath(newabs,sizeof(newabs),newPath)))
		SYSC_ERROR(stack,-EFAULT);

	int res = VFS::rename(pid,oldabs,newabs);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::mkdir(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	pid_t pid = Proc::getRunning();
	const char *path = (const char*)SYSC_ARG1(stack);
	mode_t mode = (mode_t)SYSC_ARG2(stack);
	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	int res = VFS::mkdir(pid,abspath,mode);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}

int Syscalls::rmdir(Thread *t,IntrptStackFrame *stack) {
	char abspath[MAX_PATH_LEN + 1];
	pid_t pid = t->getProc()->getPid();
	const char *path = (const char*)SYSC_ARG1(stack);
	if(EXPECT_FALSE(!copyPath(abspath,sizeof(abspath),path)))
		SYSC_ERROR(stack,-EFAULT);

	int res = VFS::rmdir(pid,abspath);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RET1(stack,res);
}
