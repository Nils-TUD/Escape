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
	SYSC_RESULT(stack,fd);
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
	SYSC_RESULT(stack,res);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,res);
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
	SYSC_RESULT(stack,readBytes);
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
	SYSC_RESULT(stack,writtenBytes);
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
	if(EXPECT_FALSE(!file->isDevice() && isDeviceMsg(id & 0xFFFF))) {
		FileDesc::release(file);
		SYSC_ERROR(stack,-EPERM);
	}

	/* send msg */
	ssize_t res = file->sendMsg(p->getPid(),id,data,size,NULL,0);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
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
	SYSC_RESULT(stack,res);
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
	if(EXPECT_FALSE(!file->isDevice() && isDeviceMsg(mid & 0xFFFF))) {
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
	SYSC_RESULT(stack,res);
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
	SYSC_RESULT(stack,res);
}

int Syscalls::delegate(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int dev = (int)SYSC_ARG1(stack);
	int fd = (int)SYSC_ARG2(stack);
	uint perm = (int)SYSC_ARG3(stack);
	int arg = (int)SYSC_ARG4(stack);
	Proc *p = t->getProc();

	/* get file to delegate */
	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* only downgrading is allowed and only O_ACCMODE */
	perm &= O_ACCMODE & file->getFlags();
	if(perm == 0) {
		FileDesc::release(file);
		SYSC_ERROR(stack,-EPERM);
	}

	/* get channel file */
	OpenFile *chanfile = FileDesc::request(p,dev);
	if(EXPECT_FALSE(chanfile == NULL)) {
		FileDesc::release(file);
		SYSC_ERROR(stack,-EBADF);
	}

	/* delegate file */
	int res = chanfile->delegate(p->getPid(),file,perm,arg);
	FileDesc::release(chanfile);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,0);
}

int Syscalls::obtain(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int dev = (int)SYSC_ARG1(stack);
	int arg = (int)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	/* get channel file */
	OpenFile *chanfile = FileDesc::request(p,dev);
	if(EXPECT_FALSE(chanfile == NULL))
		SYSC_ERROR(stack,-EBADF);

	/* obtain file */
	int res = chanfile->obtain(p->getPid(),arg);
	FileDesc::release(chanfile);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::dup(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int fd = (int)SYSC_ARG1(stack);

	int res = FileDesc::dup(fd);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::redirect(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	int src = (int)SYSC_ARG1(stack);
	int dst = (int)SYSC_ARG2(stack);

	int err = FileDesc::redirect(src,dst);
	if(EXPECT_FALSE(err < 0))
		SYSC_ERROR(stack,err);
	SYSC_RESULT(stack,err);
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
	SYSC_RESULT(stack,0);
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
	SYSC_RESULT(stack,res);
}

int Syscalls::link(Thread *t,IntrptStackFrame *stack) {
	char filename[MAX_PATH_LEN + 1];
	int target = (int)SYSC_ARG1(stack);
	int dir = (int)SYSC_ARG2(stack);
	const char *name = (const char *)SYSC_ARG3(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!copyPath(filename,sizeof(filename),name)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *targetFile = FileDesc::request(p,target);
	if(EXPECT_FALSE(targetFile == NULL))
		SYSC_ERROR(stack,-EBADF);

	OpenFile *dirFile = FileDesc::request(p,dir);
	if(EXPECT_FALSE(dirFile == NULL)) {
		FileDesc::release(targetFile);
		SYSC_ERROR(stack,-EBADF);
	}

	int res = targetFile->link(p->getPid(),dirFile,filename);
	FileDesc::release(dirFile);
	FileDesc::release(targetFile);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::unlink(Thread *t,IntrptStackFrame *stack) {
	char filename[MAX_PATH_LEN + 1];
	int fd = (int)SYSC_ARG1(stack);
	const char *name = (const char *)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!copyPath(filename,sizeof(filename),name)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *dir = FileDesc::request(p,fd);
	if(EXPECT_FALSE(dir == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = dir->unlink(p->getPid(),filename);
	FileDesc::release(dir);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::rename(Thread *t,IntrptStackFrame *stack) {
	char oldFilename[MAX_PATH_LEN + 1];
	char newFilename[MAX_PATH_LEN + 1];
	int oldDir = (int)SYSC_ARG1(stack);
	const char *oldName = (const char *)SYSC_ARG2(stack);
	int newDir = (int)SYSC_ARG3(stack);
	const char *newName = (const char *)SYSC_ARG4(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!copyPath(oldFilename,sizeof(oldFilename),oldName)))
		SYSC_ERROR(stack,-EFAULT);
	if(EXPECT_FALSE(!copyPath(newFilename,sizeof(newFilename),newName)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *oldf = FileDesc::request(p,oldDir);
	if(EXPECT_FALSE(oldf == NULL))
		SYSC_ERROR(stack,-EBADF);

	OpenFile *newf = FileDesc::request(p,newDir);
	if(EXPECT_FALSE(newf == NULL)) {
		FileDesc::release(oldf);
		SYSC_ERROR(stack,-EBADF);
	}

	int res = oldf->rename(p->getPid(),oldFilename,newf,newFilename);
	FileDesc::release(newf);
	FileDesc::release(oldf);

	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::mkdir(A_UNUSED Thread *t,IntrptStackFrame *stack) {
	char filename[MAX_PATH_LEN + 1];
	int fd = (int)SYSC_ARG1(stack);
	const char *name = (const char *)SYSC_ARG2(stack);
	mode_t mode = (mode_t)SYSC_ARG3(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!copyPath(filename,sizeof(filename),name)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->mkdir(p->getPid(),filename,mode);
	FileDesc::release(file);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}

int Syscalls::rmdir(Thread *t,IntrptStackFrame *stack) {
	char filename[MAX_PATH_LEN + 1];
	int fd = (int)SYSC_ARG1(stack);
	const char *name = (const char *)SYSC_ARG2(stack);
	Proc *p = t->getProc();

	if(EXPECT_FALSE(!copyPath(filename,sizeof(filename),name)))
		SYSC_ERROR(stack,-EFAULT);

	OpenFile *file = FileDesc::request(p,fd);
	if(EXPECT_FALSE(file == NULL))
		SYSC_ERROR(stack,-EBADF);

	int res = file->rmdir(p->getPid(),filename);
	if(EXPECT_FALSE(res < 0))
		SYSC_ERROR(stack,res);
	SYSC_RESULT(stack,res);
}
