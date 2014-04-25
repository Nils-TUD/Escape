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
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/virtmem.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/fs.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/openfile.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <sys/cppsupport.h>
#include <sys/config.h>
#include <ipc/ipcbuf.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errno.h>

static int communicate(pid_t pid,OpenFile *fsFile,msgid_t cmd,ipc::IPCBuf &ib) {
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

int VFSFS::stat(pid_t pid,OpenFile *fsFile,const char *path,sFileInfo *info) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path);

	ssize_t res = communicate(pid,fsFile,MSG_FS_STAT,ib);
	if(res < 0)
		return res;

	ib >> *info;
	return 0;
}

int VFSFS::chmod(pid_t pid,OpenFile *fsFile,const char *path,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path) << mode;
	return communicate(pid,fsFile,MSG_FS_CHMOD,ib);
}

int VFSFS::chown(pid_t pid,OpenFile *fsFile,const char *path,uid_t uid,gid_t gid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path) << uid << gid;
	return communicate(pid,fsFile,MSG_FS_CHOWN,ib);
}

int VFSFS::link(pid_t pid,OpenFile *fsFile,const char *oldPath,const char *newPath) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(oldPath) << ipc::CString(newPath);
	return communicate(pid,fsFile,MSG_FS_LINK,ib);
}

int VFSFS::unlink(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path);
	return communicate(pid,fsFile,MSG_FS_UNLINK,ib);
}

int VFSFS::mkdir(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path);
	return communicate(pid,fsFile,MSG_FS_MKDIR,ib);
}

int VFSFS::rmdir(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	ipc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << ipc::CString(path);
	return communicate(pid,fsFile,MSG_FS_RMDIR,ib);
}
