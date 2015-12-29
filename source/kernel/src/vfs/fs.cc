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
#include <esc/proto/fs.h>
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

	errcode_t err;
	ib >> err;
	return ib.error() ? -EINVAL : err;
}

int VFSFS::fstat(pid_t pid,VFSChannel *chan,struct stat *info) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	int res = communicateOverChan(pid,chan,esc::FSStat::MSG,ib);
	ib >> *info;
	/* set device id */
	info->st_dev = chan->getParent()->getNo();
	return res;
}

int VFSFS::truncate(pid_t pid,VFSChannel *chan,off_t length) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSTruncate::Request(user,length);
	return communicateOverChan(pid,chan,esc::FSTruncate::MSG,ib);
}

int VFSFS::syncfs(pid_t pid,VFSChannel *chan) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	return communicateOverChan(pid,chan,esc::FSSync::MSG,ib);
}

int VFSFS::chmod(pid_t pid,VFSChannel *chan,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSChmod::Request(user,mode);
	return communicateOverChan(pid,chan,esc::FSChmod::MSG,ib);
}

int VFSFS::chown(pid_t pid,VFSChannel *chan,uid_t uid,gid_t gid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSChown::Request(user,uid,gid);
	return communicateOverChan(pid,chan,esc::FSChown::MSG,ib);
}

int VFSFS::utime(pid_t pid,VFSChannel *chan,const struct utimbuf *utimes) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSUtime::Request(user,*utimes);
	return communicateOverChan(pid,chan,esc::FSUtime::MSG,ib);
}

int VFSFS::link(pid_t pid,VFSChannel *target,VFSChannel *dir,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSLink::Request(user,dir->getFd(),esc::CString(name));
	return communicateOverChan(pid,target,esc::FSLink::MSG,ib);
}

int VFSFS::unlink(pid_t pid,VFSChannel *chan,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSUnlink::Request(user,esc::CString(name));
	return communicateOverChan(pid,chan,esc::FSUnlink::MSG,ib);
}

int VFSFS::rename(pid_t pid,VFSChannel *oldDir,const char *oldName,VFSChannel *newDir,
		const char *newName) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSRename::Request(user,esc::CString(oldName),newDir->getFd(),esc::CString(newName));
	return communicateOverChan(pid,oldDir,esc::FSRename::MSG,ib);
}

int VFSFS::mkdir(pid_t pid,VFSChannel *chan,const char *name,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSMkdir::Request(user,esc::CString(name),mode);
	return communicateOverChan(pid,chan,esc::FSMkdir::MSG,ib);
}

int VFSFS::rmdir(pid_t pid,VFSChannel *chan,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	const Proc *p = Proc::getByPid(pid);
	fs::User user(p->getEUid(),p->getEGid(),p->getPid());
	ib << esc::FSRmdir::Request(user,esc::CString(name));
	return communicateOverChan(pid,chan,esc::FSRmdir::MSG,ib);
}
