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
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <string.h>
#include <errno.h>

#define FS_PATH				"/dev/fs"

klock_t VFSFS::fsChanLock;

void VFSFS::removeProc(pid_t pid) {
	Proc *p = Proc::getByPid(pid);
	for(auto it = p->fsChans.begin(); it != p->fsChans.end(); ) {
		auto old = it++;
		old->file->close(pid);
		delete &*old;
	}
}

int VFSFS::openPath(pid_t pid,uint flags,const char *path,OpenFile **file) {
	const Proc *p = Proc::getByPid(pid);
	ssize_t res = -ENOMEM;
	size_t pathLen = strlen(path);
	sStrMsg msg;
	inode_t inode;
	dev_t dev;

	if(pathLen > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;
	VFSNode *node;
	OpenFile *fs;
	res = requestFile(pid,&node,&fs);
	if(res < 0)
		return res;

	/* send msg to fs */
	msg.arg1 = flags;
	msg.arg2 = p->getEUid();
	msg.arg3 = p->getEGid();
	msg.arg4 = p->getPid();
	memcpy(msg.s1,path,pathLen + 1);
	res = fs->sendMsg(pid,MSG_FS_OPEN,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* receive response */
	do
		res = fs->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;

	inode = msg.arg1;
	dev = msg.arg2;
	if(inode < 0) {
		res = inode;
		goto error;
	}

	/* now open the file */
	res = VFS::openFile(pid,flags,NULL,inode,dev,file);

error:
	releaseFile(pid,fs);
	return res;
}

int VFSFS::istat(pid_t pid,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	return doStat(pid,NULL,ino,devNo,info);
}

int VFSFS::stat(pid_t pid,const char *path,USER sFileInfo *info) {
	return doStat(pid,path,0,0,info);
}

int VFSFS::doStat(pid_t pid,const char *path,inode_t ino,dev_t devNo,USER sFileInfo *info) {
	ssize_t res = -ENOMEM;
	size_t pathLen = 0;
	sMsg msg;

	if(path) {
		pathLen = strlen(path);
		if(pathLen > MAX_MSGSTR_LEN)
			return -ENAMETOOLONG;
	}
	OpenFile *fs;
	VFSNode *node;
	res = requestFile(pid,&node,&fs);
	if(res < 0)
		return res;

	/* send msg to fs */
	if(path) {
		const Proc *p = Proc::getByPid(pid);
		msg.str.arg1 = p->getEUid();
		msg.str.arg2 = p->getEGid();
		msg.str.arg3 = p->getPid();
		memcpy(msg.str.s1,path,pathLen + 1);
		res = fs->sendMsg(pid,MSG_FS_STAT,&msg,sizeof(msg.str),NULL,0);
	}
	else {
		msg.args.arg1 = ino;
		msg.args.arg2 = devNo;
		res = fs->sendMsg(pid,MSG_FS_ISTAT,&msg,sizeof(msg.args),NULL,0);
	}
	if(res < 0)
		goto error;

	/* receive response */
	do
		res = fs->receiveMsg(pid,NULL,&msg,sizeof(msg.data),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.data.arg1;

	/* copy file-info */
	if(res == 0)
		memcpy(info,msg.data.d,sizeof(sFileInfo));

error:
	releaseFile(pid,fs);
	return res;
}

ssize_t VFSFS::read(pid_t pid,inode_t inodeNo,dev_t devNo,USER void *buffer,off_t offset,
		size_t count) {
	sArgsMsg msg;
	VFSNode *node;
	OpenFile *fs;
	ssize_t res = requestFile(pid,&node,&fs);
	if(res < 0)
		return res;

	/* send msg to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = fs->sendMsg(pid,MSG_FS_READ,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = fs->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;

	/* handle response */
	res = msg.arg1;
	if(res <= 0)
		goto error;

	/* read data */
	do
		res = fs->receiveMsg(pid,NULL,buffer,count,true);
	while(res == -EINTR);

error:
	releaseFile(pid,fs);
	return res;
}

ssize_t VFSFS::write(pid_t pid,inode_t inodeNo,dev_t devNo,USER const void *buffer,off_t offset,
		size_t count) {
	sArgsMsg msg;
	VFSNode *node;
	OpenFile *fs;
	ssize_t res = requestFile(pid,&node,&fs);
	if(res < 0)
		return res;

	/* send msg and data */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	msg.arg3 = offset;
	msg.arg4 = count;
	res = fs->sendMsg(pid,MSG_FS_WRITE,&msg,sizeof(msg),buffer,count);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = fs->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.arg1;

error:
	releaseFile(pid,fs);
	return res;
}

int VFSFS::chmod(pid_t pid,const char *path,mode_t mode) {
	return pathReqHandler(pid,path,NULL,mode,MSG_FS_CHMOD);
}

int VFSFS::chown(pid_t pid,const char *path,uid_t uid,gid_t gid) {
	/* TODO better solution? */
	return pathReqHandler(pid,path,NULL,(uid << 16) | (gid & 0xFFFF),MSG_FS_CHOWN);
}

int VFSFS::link(pid_t pid,const char *oldPath,const char *newPath) {
	return pathReqHandler(pid,oldPath,newPath,0,MSG_FS_LINK);
}

int VFSFS::unlink(pid_t pid,const char *path) {
	return pathReqHandler(pid,path,NULL,0,MSG_FS_UNLINK);
}

int VFSFS::mkdir(pid_t pid,const char *path) {
	return pathReqHandler(pid,path,NULL,0,MSG_FS_MKDIR);
}

int VFSFS::rmdir(pid_t pid,const char *path) {
	return pathReqHandler(pid,path,NULL,0,MSG_FS_RMDIR);
}

int VFSFS::mount(pid_t pid,const char *device,const char *path,uint type) {
	return pathReqHandler(pid,device,path,type,MSG_FS_MOUNT);
}

int VFSFS::unmount(pid_t pid,const char *path) {
	return pathReqHandler(pid,path,NULL,0,MSG_FS_UNMOUNT);
}

int VFSFS::sync(pid_t pid) {
	OpenFile *fs;
	int res = requestFile(pid,NULL,&fs);
	if(res < 0)
		return res;
	res = fs->sendMsg(pid,MSG_FS_SYNC,NULL,0,NULL,0);
	releaseFile(pid,fs);
	return res;
}

void VFSFS::close(pid_t pid,inode_t inodeNo,dev_t devNo) {
	sArgsMsg msg;
	OpenFile *fs;
	int res = requestFile(pid,NULL,&fs);
	if(res < 0)
		return;

	/* write message to fs */
	msg.arg1 = inodeNo;
	msg.arg2 = devNo;
	fs->sendMsg(pid,MSG_FS_CLOSE,&msg,sizeof(msg),NULL,0);
	/* no response necessary, therefore no wait, too */
	releaseFile(pid,fs);
}

int VFSFS::pathReqHandler(pid_t pid,const char *path1,const char *path2,uint arg1,uint cmd) {
	int res = -ENOMEM;
	const Proc *p = Proc::getByPid(pid);
	sStrMsg msg;

	if(strlen(path1) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;
	if(path2 && strlen(path2) > MAX_MSGSTR_LEN)
		return -ENAMETOOLONG;

	OpenFile *fs;
	VFSNode *node;
	res = requestFile(pid,&node,&fs);
	if(res < 0)
		return res;

	/* send msg */
	strcpy(msg.s1,path1);
	if(path2)
		strcpy(msg.s2,path2);
	msg.arg1 = arg1;
	msg.arg2 = p->getEUid();
	msg.arg3 = p->getEGid();
	msg.arg4 = p->getPid();
	res = fs->sendMsg(pid,cmd,&msg,sizeof(msg),NULL,0);
	if(res < 0)
		goto error;

	/* read response */
	do
		res = fs->receiveMsg(pid,NULL,&msg,sizeof(msg),true);
	while(res == -EINTR);
	if(res < 0)
		goto error;
	res = msg.arg1;

error:
	releaseFile(pid,fs);
	return res;
}

int VFSFS::requestFile(pid_t pid,VFSNode **node,OpenFile **file) {
	Proc *p = Proc::getByPid(pid);
	/* check if there's a free channel */
	SpinLock::acquire(&fsChanLock);
	for(auto it = p->fsChans.begin(); it != p->fsChans.end(); ++it) {
		if(!it->active) {
			if(!it->file->getNode()->isAlive()) {
				/* remove channel */
				p->fsChans.remove(&*it);
				delete &*it;
				SpinLock::release(&fsChanLock);
				return -EDESTROYED;
			}
			if(node)
				*node = it->file->getNode();
			it->active = true;
			SpinLock::release(&fsChanLock);
			*file = it->file;
			return 0;
		}
	}
	SpinLock::release(&fsChanLock);

	FSChan *chan = new FSChan();
	if(!chan)
		return -ENOMEM;

	int err = VFS::openPath(p->getPid(),VFS_MSGS,0,FS_PATH,&chan->file);
	if(err < 0) {
		Cache::free(chan);
		return err;
	}

	SpinLock::acquire(&fsChanLock);
	p->fsChans.append(chan);
	SpinLock::release(&fsChanLock);
	if(node)
		*node = chan->file->getNode();
	*file = chan->file;
	return 0;
}

void VFSFS::releaseFile(pid_t pid,OpenFile *file) {
	Proc *p = Proc::getByPid(pid);
	SpinLock::acquire(&fsChanLock);
	for(auto it = p->fsChans.begin(); it != p->fsChans.end(); ++it) {
		if(it->file == file) {
			it->active = false;
			break;
		}
	}
	SpinLock::release(&fsChanLock);
}

void VFSFS::printFSChans(OStream &os,const Proc *p) {
	SpinLock::acquire(&fsChanLock);
	os.writef("FS-Channels:\n");
	for(auto it = p->fsChans.cbegin(); it != p->fsChans.cend(); ++it)
		os.writef("\t%s (%s)\n",it->file->getNode()->getPath(),it->active ? "active" : "not active");
	SpinLock::release(&fsChanLock);
}
