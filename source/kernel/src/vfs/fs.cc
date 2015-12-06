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

#include <esc/ipc/ipcbuf.h>
#include <mem/cache.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <sys/messages.h>
#include <task/proc.h>
#include <vfs/channel.h>
#include <vfs/fs.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <common.h>
#include <config.h>
#include <cppsupport.h>
#include <errno.h>
#include <spinlock.h>
#include <string.h>
#include <util.h>
#include <video.h>

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

static int communicateOverChan(pid_t pid,VFSChannel *chan,msgid_t cmd,esc::IPCBuf &ib) {
	ssize_t res;
	if(ib.error())
		return -EINVAL;

	/* send msg */
	res = chan->send(pid,0,cmd,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	msgid_t mid = res;
	res = chan->receive(pid,0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		return res;

	int err;
	ib >> err;
	return ib.error() ? -EINVAL : err;
}

int VFSFS::fstat(pid_t pid,VFSChannel *chan,struct stat *info) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	int res = communicateOverChan(pid,chan,MSG_FS_ISTAT,ib);
	ib >> *info;
	/* set device id */
	info->st_dev = chan->getParent()->getNo();
	return res;
}

int VFSFS::truncate(pid_t pid,VFSChannel *chan,off_t length) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << length;
	return communicateOverChan(pid,chan,MSG_FS_TRUNCATE,ib);
}

int VFSFS::syncfs(pid_t pid,VFSChannel *chan) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	return communicateOverChan(pid,chan,MSG_FS_SYNCFS,ib);
}

int VFSFS::chmod(pid_t pid,VFSChannel *chan,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << mode;
	return communicateOverChan(pid,chan,MSG_FS_CHMOD,ib);
}

int VFSFS::chown(pid_t pid,VFSChannel *chan,uid_t uid,gid_t gid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << uid << gid;
	return communicateOverChan(pid,chan,MSG_FS_CHOWN,ib);
}

int VFSFS::utime(pid_t pid,VFSChannel *chan,const struct utimbuf *utimes) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << *utimes;
	return communicateOverChan(pid,chan,MSG_FS_UTIME,ib);
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

int VFSFS::mkdir(pid_t pid,VFSChannel *chan,const char *name,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(name) << mode;
	return communicateOverChan(pid,chan,MSG_FS_MKDIR,ib);
}

int VFSFS::rmdir(pid_t pid,OpenFile *fsFile,const char *path) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	ib << p->getEUid() << p->getEGid() << p->getPid() << esc::CString(path);
	return communicate(pid,fsFile,MSG_FS_RMDIR,ib);
}
