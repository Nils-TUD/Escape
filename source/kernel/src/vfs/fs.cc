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

#include <common.h>
#include <task/proc.h>
#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <vfs/vfs.h>
#include <vfs/node.h>
#include <vfs/fs.h>
#include <vfs/channel.h>
#include <vfs/openfile.h>
#include <util.h>
#include <video.h>
#include <spinlock.h>
#include <cppsupport.h>
#include <config.h>
#include <esc/ipc/ipcbuf.h>
#include <sys/messages.h>
#include <string.h>
#include <errno.h>

static int communicate(pid_t pid,OpenFile *fsFile,msgid_t cmd,esc::IPCBuf &ib) {
	ssize_t res;
	if(ib.error())
		return -EINVAL;

	/* send msg */
	res = fsFile->sendMsg(pid,cmd,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	msgid_t mid = res;
	res = fsFile->receiveMsg(pid,&mid,ib.buffer(),ib.max(),0);
	if(res < 0)
		return res;

	int err;
	ib >> err;
	return ib.error() ? -EINVAL : err;
}

int VFSFS::stat(pid_t pid,OpenFile *fsFile,const char *path,struct stat *info) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path);

	ssize_t res = communicate(pid,fsFile,MSG_FS_STAT,ib);
	if(res < 0)
		return res;

	ib >> *info;
	return 0;
}

int VFSFS::chmod(pid_t pid,OpenFile *fsFile,const char *path,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path) << mode;
	return communicate(pid,fsFile,MSG_FS_CHMOD,ib);
}

int VFSFS::chown(pid_t pid,OpenFile *fsFile,const char *path,uid_t uid,gid_t gid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path) << uid << gid;
	return communicate(pid,fsFile,MSG_FS_CHOWN,ib);
}

int VFSFS::utime(pid_t pid,OpenFile *fsFile,const char *path,const struct utimbuf *utimes) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path) << *utimes;
	return communicate(pid,fsFile,MSG_FS_UTIME,ib);
}

int VFSFS::link(pid_t pid,OpenFile *fsFile,const char *oldPath,const char *newPath) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(oldPath) << esc::CString(newPath);
	return communicate(pid,fsFile,MSG_FS_LINK,ib);
}

int VFSFS::unlink(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path);
	return communicate(pid,fsFile,MSG_FS_UNLINK,ib);
}

int VFSFS::rename(pid_t pid,OpenFile *fsFile,const char *oldPath,const char *newPath) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(oldPath) << esc::CString(newPath);
	return communicate(pid,fsFile,MSG_FS_RENAME,ib);
}

int VFSFS::mkdir(pid_t pid,OpenFile *fsFile,const char *path,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path) << mode;
	return communicate(pid,fsFile,MSG_FS_MKDIR,ib);
}

int VFSFS::rmdir(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path);
	return communicate(pid,fsFile,MSG_FS_RMDIR,ib);
}
