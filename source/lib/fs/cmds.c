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

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <fs/cmds.h>
#include <fs/threadpool.h>
#include <fs/fsdev.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "files.h"

static void cmds_open(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_read(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_write(sFileSystem *fs,int fd,sMsg *msg,void *data);
static void cmds_close(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_stat(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_syncfs(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_link(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_unlink(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_mkdir(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_rmdir(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_istat(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_chmod(sFileSystem *fs,int fd,sMsg *msg);
static void cmds_chown(sFileSystem *fs,int fd,sMsg *msg);

static fReqHandler commands[] = {
	/* MSG_FS_OPEN */		(fReqHandler)cmds_open,
	/* MSG_FS_READ */		(fReqHandler)cmds_read,
	/* MSG_FS_WRITE */		(fReqHandler)cmds_write,
	/* MSG_FS_CLOSE */		(fReqHandler)cmds_close,
	/* MSG_FS_STAT */		(fReqHandler)cmds_stat,
	/* MSG_FS_SYNCFS */		(fReqHandler)cmds_syncfs,
	/* MSG_FS_LINK */		(fReqHandler)cmds_link,
	/* MSG_FS_UNLINK */		(fReqHandler)cmds_unlink,
	/* MSG_FS_MKDIR */		(fReqHandler)cmds_mkdir,
	/* MSG_FS_RMDIR */		(fReqHandler)cmds_rmdir,
	/* MSG_FS_ISTAT */		(fReqHandler)cmds_istat,
	/* MSG_FS_CHMOD */		(fReqHandler)cmds_chmod,
	/* MSG_FS_CHOWN */		(fReqHandler)cmds_chown,
};

bool cmds_execute(sFileSystem *fs,int cmd,int fd,sMsg *msg,void *data) {
	if(cmd < MSG_FS_OPEN || cmd - MSG_FS_OPEN >= (int)ARRAY_SIZE(commands))
		return false;

	/* TODO don't use the preprocessor for that! */
#if REQ_THREAD_COUNT > 0
	if(!tpool_addRequest(commands[cmd - MSG_FS_OPEN],fs,fd,msg,sizeof(sMsg),data))
		printe("Not enough mem for request %d",cmd);
#else
	commands[cmd - MSG_FS_OPEN](fs,fd,msg,data);
#endif
	return true;
}

static void cmds_open(sFileSystem *fs,int fd,sMsg *msg) {
	uint flags = msg->str.arg1;
	inode_t no;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	no = fs->resPath(fs->handle,&u,msg->str.s1,flags);
	if(no >= 0) {
		msg->args.arg1 = fs->open(fs->handle,&u,no,flags);
		if((int)msg->args.arg1 >= 0) {
			int res = files_open(fd,no);
			if(res < 0) {
				fs->close(fs->handle,no);
				msg->args.arg1 = res;
			}
		}
	}
	else
		msg->args.arg1 = -ENOENT;
	send(fd,MSG_FS_OPEN_RESP,msg,sizeof(msg->args));
}

static void cmds_istat(sFileSystem *fs,int fd,sMsg *msg) {
	FSFile *file = files_get(fd);
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	msg->data.arg1 = fs->stat(fs->handle,file->ino,info);
	send(fd,MSG_FS_STAT_RESP,msg,sizeof(msg->data));
}

static void cmds_read(sFileSystem *fs,int fd,sMsg *msg) {
	FSFile *file = files_get(fd);
	uint offset = msg->args.arg1;
	size_t count = msg->args.arg2;
	void *buffer = NULL;
	if(fs->read == NULL)
		msg->args.arg1 = -ENOTSUP;
	else {
		buffer = malloc(count);
		if(buffer == NULL)
			msg->args.arg1 = -ENOMEM;
		else
			msg->args.arg1 = fs->read(fs->handle,file->ino,buffer,offset,count);
	}
	send(fd,MSG_DEV_READ_RESP,msg,sizeof(msg->args));
	if(buffer) {
		if(msg->args.arg1 > 0)
			send(fd,MSG_DEV_READ_RESP,buffer,msg->args.arg1);
		free(buffer);
	}
}

static void cmds_write(sFileSystem *fs,int fd,sMsg *msg,void *data) {
	FSFile *file = files_get(fd);
	uint offset = msg->args.arg1;
	size_t count = msg->args.arg2;
	if(fs->readonly)
		msg->args.arg1 = -EROFS;
	else if(fs->write == NULL)
		msg->args.arg1 = -ENOTSUP;
	/* write to file */
	else
		msg->args.arg1 = fs->write(fs->handle,file->ino,data,offset,count);
	/* send response */
	send(fd,MSG_DEV_WRITE_RESP,msg,sizeof(msg->args));
}

static void cmds_close(sFileSystem *fs,int fd,A_UNUSED sMsg *msg) {
	FSFile *file = files_get(fd);
	fs->close(fs->handle,file->ino);
	files_close(fd);
	close(fd);
}

static void cmds_stat(sFileSystem *fs,int fd,sMsg *msg) {
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	inode_t no;
	sFSUser u;
	u.uid = msg->str.arg1;
	u.gid = msg->str.arg2;
	u.pid = msg->str.arg3;
	no = fs->resPath(fs->handle,&u,msg->str.s1,IO_READ);
	if(no < 0)
		msg->data.arg1 = no;
	else
		msg->data.arg1 = fs->stat(fs->handle,no,info);
	send(fd,MSG_FS_STAT_RESP,msg,sizeof(msg->data));
}

static void cmds_chmod(sFileSystem *fs,int fd,sMsg *msg) {
	char *path = msg->str.s1;
	mode_t mode = msg->str.arg1;
	inode_t ino;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = fs->resPath(fs->handle,&u,path,IO_READ);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		if(fs->readonly)
			msg->args.arg1 = -EROFS;
		else if(fs->chmod == NULL)
			msg->args.arg1 = -ENOTSUP;
		else
			msg->args.arg1 = fs->chmod(fs->handle,&u,ino,mode);
	}
	send(fd,MSG_FS_CHMOD_RESP,msg,sizeof(msg->args));
}

static void cmds_chown(sFileSystem *fs,int fd,sMsg *msg) {
	char *path = msg->str.s1;
	uid_t uid = (uid_t)(msg->str.arg1 >> 16);
	gid_t gid = (gid_t)(msg->str.arg1 & 0xFFFF);
	inode_t ino;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = fs->resPath(fs->handle,&u,path,IO_READ);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		if(fs->readonly)
			msg->args.arg1 = -EROFS;
		else if(fs->chown == NULL)
			msg->args.arg1 = -ENOTSUP;
		else
			msg->args.arg1 = fs->chown(fs->handle,&u,ino,uid,gid);
	}
	send(fd,MSG_FS_CHOWN_RESP,msg,sizeof(msg->args));
}

static char *splitPath(char *path) {
	/* split path and name */
	size_t len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	char *name = strrchr(path,'/');
	if(name) {
		name = strdup(name + 1);
		dirname(path);
	}
	else {
		name = strdup(path);
		/* we know that path resides in sStrMsg */
		strcpy(path,"/");
	}
	return name;
}

static void cmds_link(sFileSystem *fs,int fd,sMsg *msg) {
	char *oldPath = msg->str.s1;
	char *newPath = msg->str.s2;
	inode_t dirIno,dstIno;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	dstIno = fs->resPath(fs->handle,&u,oldPath,IO_READ);
	if(dstIno < 0)
		msg->args.arg1 = dstIno;
	else {
		char *name = splitPath(newPath);
		msg->args.arg1 = -ENOMEM;
		if(name) {
			dirIno = fs->resPath(fs->handle,&u,newPath,IO_READ);
			if(dirIno < 0)
				msg->args.arg1 = dirIno;
			else if(fs->readonly)
				msg->args.arg1 = -EROFS;
			else if(fs->link == NULL)
				msg->args.arg1 = -ENOTSUP;
			else
				msg->args.arg1 = fs->link(fs->handle,&u,dstIno,dirIno,name);
			free(name);
		}
	}
	send(fd,MSG_FS_LINK_RESP,msg,sizeof(msg->args));
}

static void cmds_unlink(sFileSystem *fs,int fd,sMsg *msg) {
	char *path = msg->str.s1;
	inode_t dirIno;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	dirIno = fs->resPath(fs->handle,&u,path,IO_READ);
	if(dirIno < 0)
		msg->args.arg1 = dirIno;
	else {
		char *name = splitPath(path);
		msg->args.arg1 = -ENOMEM;
		if(name) {
			/* get directory */
			dirIno = fs->resPath(fs->handle,&u,path,IO_READ);
			vassert(dirIno >= 0,"Subdir found, but parent not!?");
			if(fs->readonly)
				msg->args.arg1 = -EROFS;
			else if(fs->unlink == NULL)
				msg->args.arg1 = -ENOTSUP;
			else
				msg->args.arg1 = fs->unlink(fs->handle,&u,dirIno,name);
			free(name);
		}
	}
	send(fd,MSG_FS_UNLINK_RESP,msg,sizeof(msg->args));
}

static void cmds_mkdir(sFileSystem *fs,int fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name;
	inode_t dirIno;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	name = splitPath(path);
	msg->args.arg1 = -ENOMEM;
	if(name) {
		dirIno = fs->resPath(fs->handle,&u,path,IO_READ);
		if(dirIno < 0)
			msg->args.arg1 = dirIno;
		else {
			if(fs->readonly)
				msg->args.arg1 = -EROFS;
			else if(fs->mkdir == NULL)
				msg->args.arg1 = -ENOTSUP;
			else
				msg->args.arg1 = fs->mkdir(fs->handle,&u,dirIno,name);
		}
		free(name);
	}
	send(fd,MSG_FS_MKDIR_RESP,msg,sizeof(msg->args));
}

static void cmds_rmdir(sFileSystem *fs,int fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name;
	inode_t dirIno;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	name = splitPath(path);
	msg->args.arg1 = -ENOMEM;
	if(name) {
		dirIno = fs->resPath(fs->handle,&u,path,IO_READ);
		if(dirIno < 0)
			msg->args.arg1 = dirIno;
		else {
			if(fs->readonly)
				msg->args.arg1 = -EROFS;
			else if(fs->rmdir == NULL)
				msg->args.arg1 = -ENOTSUP;
			else
				msg->args.arg1 = fs->rmdir(fs->handle,&u,dirIno,name);
		}
		free(name);
	}
	send(fd,MSG_FS_RMDIR_RESP,msg,sizeof(msg->args));
}

static void cmds_syncfs(sFileSystem *fs,A_UNUSED int fd,A_UNUSED sMsg *msg) {
	if(fs->sync)
		fs->sync(fs->handle);
	msg->args.arg1 = 0;
	send(fd,MSG_FS_SYNCFS_RESP,msg,sizeof(msg->args));
}
