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

static int communicateOverChan(VFSChannel *chan,msgid_t cmd,esc::IPCBuf &ib) {
	ssize_t res;
	if(ib.error())
		return -EINVAL;

	/* send msg */
	res = chan->send(0,cmd,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	ib.reset();
	msgid_t mid = res;
	res = chan->receive(0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		return res;

	errcode_t err;
	ib >> err;
	return ib.error() ? -EINVAL : err;
}

int VFSFS::fstat(VFSChannel *chan,struct stat *info) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	int res = communicateOverChan(chan,esc::FSStat::MSG,ib);
	ib >> *info;
	/* set device id */
	info->st_dev = chan->getParent()->getNo();
	return res;
}

int VFSFS::truncate(VFSChannel *chan,off_t length) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSTruncate::Request(length);
	return communicateOverChan(chan,esc::FSTruncate::MSG,ib);
}

int VFSFS::syncfs(VFSChannel *chan) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	return communicateOverChan(chan,esc::FSSync::MSG,ib);
}

int VFSFS::chmod(VFSChannel *chan,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSChmod::Request(mode);
	return communicateOverChan(chan,esc::FSChmod::MSG,ib);
}

int VFSFS::chown(VFSChannel *chan,uid_t uid,gid_t gid) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSChown::Request(uid,gid);
	return communicateOverChan(chan,esc::FSChown::MSG,ib);
}

int VFSFS::utime(VFSChannel *chan,const struct utimbuf *utimes) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSUtime::Request(*utimes);
	return communicateOverChan(chan,esc::FSUtime::MSG,ib);
}

int VFSFS::link(VFSChannel *target,VFSChannel *dir,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSLink::Request(dir->getFd(),esc::CString(name));
	return communicateOverChan(target,esc::FSLink::MSG,ib);
}

int VFSFS::unlink(VFSChannel *chan,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSUnlink::Request(esc::CString(name));
	return communicateOverChan(chan,esc::FSUnlink::MSG,ib);
}

int VFSFS::rename(VFSChannel *oldDir,const char *oldName,VFSChannel *newDir,
		const char *newName) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSRename::Request(esc::CString(oldName),newDir->getFd(),esc::CString(newName));
	return communicateOverChan(oldDir,esc::FSRename::MSG,ib);
}

int VFSFS::mkdir(VFSChannel *chan,const char *name,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSMkdir::Request(esc::CString(name),mode);
	return communicateOverChan(chan,esc::FSMkdir::MSG,ib);
}

int VFSFS::rmdir(VFSChannel *chan,const char *name) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSRmdir::Request(esc::CString(name));
	return communicateOverChan(chan,esc::FSRmdir::MSG,ib);
}

int VFSFS::symlink(VFSChannel *chan,const char *name,const char *target) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	ib << esc::FSSymlink::Request(esc::CString(name),esc::CString(target));
	return communicateOverChan(chan,esc::FSSymlink::MSG,ib);
}
