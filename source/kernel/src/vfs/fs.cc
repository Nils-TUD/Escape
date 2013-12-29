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
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errno.h>

int VFSFS::openPath(pid_t pid,OpenFile *fsFile,uint flags,const char *path,OpenFile **file) {
	ssize_t res = -ENOMEM;
	size_t pathLen = strlen(path);
	sStrMsg msg;
	inode_t inode;

	if(pathLen > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;

	/* send msg to fs */
	msg.arg1 = flags;
	const Proc *p = Proc::getRef(pid);
	if(p) {
		msg.arg2 = p->getEUid();
		msg.arg3 = p->getEGid();
		msg.arg4 = p->getPid();
		Proc::relRef(p);
	}
	memcpy(msg.s1,path,pathLen + 1);
	res = fsFile->sendMsg(pid,MSG_FS_OPEN,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		return res;

	inode = msg.arg1;
	if(inode < 0)
		return inode;

	/* now open the file */
	return VFS::openFile(pid,flags,reinterpret_cast<VFSNode*>(fsFile),inode,fsFile->getNodeNo(),file);
}

int VFSFS::istat(pid_t pid,OpenFile *fsFile,inode_t ino,USER sFileInfo *info) {
	return doStat(pid,fsFile,NULL,ino,info);
}

int VFSFS::stat(pid_t pid,OpenFile *fsFile,const char *path,USER sFileInfo *info) {
	return doStat(pid,fsFile,path,0,info);
}

int VFSFS::doStat(pid_t pid,OpenFile *fsFile,const char *path,inode_t ino,USER sFileInfo *info) {
	ssize_t res = -ENOMEM;
	size_t pathLen = 0;
	sMsg msg;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return -ENAMETOOLONG;
	}

	/* send msg to fs */
	if(path) {
		const Proc *p = Proc::getRef(pid);
		if(p) {
			msg.str.arg1 = p->getEUid();
			msg.str.arg2 = p->getEGid();
			msg.str.arg3 = p->getPid();
			Proc::relRef(p);
		}
		memcpy(msg.str.s1,path,pathLen + 1);
		res = fsFile->sendMsg(pid,MSG_FS_STAT,&msg,sizeof(msg.str),NULL,0);
	}
	else {
		msg.args.arg1 = ino;
		res = fsFile->sendMsg(pid,MSG_FS_ISTAT,&msg,sizeof(msg.args),NULL,0);
	}
	if(res < 0)
		return res;

	/* receive response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg.data),true);
	while(res == -EINTR);
	if(res < 0)
		return res;
	res = msg.data.arg1;

	/* copy file-info */
	if(res == 0)
		memcpy(info,msg.data.d,sizeof(sFileInfo));
	return res;
}

ssize_t VFSFS::read(pid_t pid,OpenFile *fsFile,inode_t inodeNo,USER void *buffer,off_t offset,
		size_t count) {
	sArgsMsg msg;

	/* send msg to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = offset;
	msg.arg3 = count;
	ssize_t res = fsFile->sendMsg(pid,MSG_FS_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		return res;

	/* handle response */
	res = msg.arg1;
	if(res <= 0)
		return res;

	/* read data */
	do
		res = fsFile->receiveMsg(pid,NULL,buffer,count,true);
	while(res == -EINTR);
	return res;
}

ssize_t VFSFS::write(pid_t pid,OpenFile *fsFile,inode_t inodeNo,USER const void *buffer,off_t offset,
		size_t count) {
	sArgsMsg msg;

	/* send msg and data */
	msg.arg1 = inodeNo;
	msg.arg2 = offset;
	msg.arg3 = count;
	ssize_t res = fsFile->sendMsg(pid,MSG_FS_WRITE,&msg,sizeof(msg),buffer,count);
	if(res < 0)
		return res;

	/* read response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		return res;
	return msg.arg1;
}

int VFSFS::chmod(pid_t pid,OpenFile *fsFile,const char *path,mode_t mode) {
	return pathReqHandler(pid,fsFile,path,NULL,mode,MSG_FS_CHMOD);
}

int VFSFS::chown(pid_t pid,OpenFile *fsFile,const char *path,uid_t uid,gid_t gid) {
	/* TODO better solution? */
	return pathReqHandler(pid,fsFile,path,NULL,(uid << 16) | (gid & 0xFFFF),MSG_FS_CHOWN);
}

int VFSFS::link(pid_t pid,OpenFile *fsFile,const char *oldPath,const char *newPath) {
	return pathReqHandler(pid,fsFile,oldPath,newPath,0,MSG_FS_LINK);
}

int VFSFS::unlink(pid_t pid,OpenFile *fsFile,const char *path) {
	return pathReqHandler(pid,fsFile,path,NULL,0,MSG_FS_UNLINK);
}

int VFSFS::mkdir(pid_t pid,OpenFile *fsFile,const char *path) {
	return pathReqHandler(pid,fsFile,path,NULL,0,MSG_FS_MKDIR);
}

int VFSFS::rmdir(pid_t pid,OpenFile *fsFile,const char *path) {
	return pathReqHandler(pid,fsFile,path,NULL,0,MSG_FS_RMDIR);
}

int VFSFS::syncfs(pid_t pid,OpenFile *fsFile) {
	sArgsMsg msg;
	ssize_t res = fsFile->sendMsg(pid,MSG_FS_SYNCFS,NULL,0,NULL,0);
	if(res < 0)
		return res;

	/* read response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		return res;
	return msg.arg1;
}

void VFSFS::close(pid_t pid,OpenFile *fsFile,inode_t inodeNo) {
	/* write message to fs */
	sArgsMsg msg;
	msg.arg1 = inodeNo;
	fsFile->sendMsg(pid,MSG_FS_CLOSE,&msg,sizeof(msg),NULL,0);
	/* no response necessary, therefore no wait, too */
}

int VFSFS::pathReqHandler(pid_t pid,OpenFile *fsFile,const char *path1,const char *path2,uint arg1,uint cmd) {
	int res = -ENOMEM;
	sStrMsg msg;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;

	/* send msg */
	strcpy(msg.s1,path1);
	if(path2)
		strcpy(msg.s2,path2);
	msg.arg1 = arg1;
	const Proc *p = Proc::getRef(pid);
	if(p) {
		msg.arg2 = p->getEUid();
		msg.arg3 = p->getEGid();
		msg.arg4 = p->getPid();
		Proc::relRef(p);
	}
	res = fsFile->sendMsg(pid,cmd,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		return res;

	/* read response */
	do
		res = fsFile->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		return res;
	return msg.arg1;
}
