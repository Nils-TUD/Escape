/**
 * $Id$
 */

#include <esc/common.h>
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cmds.h"
#include "threadpool.h"
#include "mount.h"

static void cmds_open(int fd,sMsg *msg);
static void cmds_read(int fd,sMsg *msg);
static void cmds_write(int fd,sMsg *msg,void *data);
static void cmds_close(int fd,sMsg *msg);
static void cmds_stat(int fd,sMsg *msg);
static void cmds_sync(int fd,sMsg *msg);
static void cmds_link(int fd,sMsg *msg);
static void cmds_unlink(int fd,sMsg *msg);
static void cmds_mkdir(int fd,sMsg *msg);
static void cmds_rmdir(int fd,sMsg *msg);
static void cmds_mount(int fd,sMsg *msg);
static void cmds_unmount(int fd,sMsg *msg);
static void cmds_istat(int fd,sMsg *msg);
static void cmds_chmod(int fd,sMsg *msg);
static void cmds_chown(int fd,sMsg *msg);

static fReqHandler commands[] = {
	/* MSG_FS_OPEN */		(fReqHandler)cmds_open,
	/* MSG_FS_READ */		(fReqHandler)cmds_read,
	/* MSG_FS_WRITE */		(fReqHandler)cmds_write,
	/* MSG_FS_CLOSE */		(fReqHandler)cmds_close,
	/* MSG_FS_STAT */		(fReqHandler)cmds_stat,
	/* MSG_FS_SYNC */		(fReqHandler)cmds_sync,
	/* MSG_FS_LINK */		(fReqHandler)cmds_link,
	/* MSG_FS_UNLINK */		(fReqHandler)cmds_unlink,
	/* MSG_FS_MKDIR */		(fReqHandler)cmds_mkdir,
	/* MSG_FS_RMDIR */		(fReqHandler)cmds_rmdir,
	/* MSG_FS_MOUNT */		(fReqHandler)cmds_mount,
	/* MSG_FS_UNMOUNT */	(fReqHandler)cmds_unmount,
	/* MSG_FS_ISTAT */		(fReqHandler)cmds_istat,
	/* MSG_FS_CHMOD */		(fReqHandler)cmds_chmod,
	/* MSG_FS_CHOWN */		(fReqHandler)cmds_chown,
};
static dev_t rootDev;
static sFSInst *root;

sFSInst *cmds_getRoot(void) {
	return root;
}

void cmds_setRoot(dev_t dev,sFSInst *fs) {
	rootDev = dev;
	root = fs;
}

bool cmds_execute(int cmd,int fd,sMsg *msg,void *data) {
	if(cmd < MSG_FS_OPEN || cmd - MSG_FS_OPEN >= (int)ARRAY_SIZE(commands))
		return false;

	/* TODO don't use the preprocessor for that! */
#if REQ_THREAD_COUNT > 0
	if(!tpool_addRequest(commands[mid - MSG_FS_OPEN],fd,msg,sizeof(msg),data))
		printf("[FS] Not enough mem for request %d\n",mid);
#else
	commands[cmd - MSG_FS_OPEN](fd,msg,data);
	close(fd);
#endif
	return true;
}

static void cmds_open(int fd,sMsg *msg) {
	dev_t devNo = rootDev;
	uint flags = msg->args.arg1;
	sFSInst *inst;
	inode_t no;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	no = root->fs->resPath(root->handle,&u,msg->str.s1,flags,&devNo,true);
	if(no >= 0) {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else
			msg->args.arg1 = inst->fs->open(inst->handle,&u,no,flags);
	}
	else
		msg->args.arg1 = ERR_PATH_NOT_FOUND;
	msg->args.arg2 = devNo;
	send(fd,MSG_FS_OPEN_RESP,msg,sizeof(msg->args));
}

static void cmds_stat(int fd,sMsg *msg) {
	dev_t devNo = rootDev;
	sFSInst *inst;
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	inode_t no;
	sFSUser u;
	u.uid = msg->str.arg1;
	u.gid = msg->str.arg2;
	u.pid = msg->str.arg3;
	no = root->fs->resPath(root->handle,&u,msg->str.s1,IO_READ,&devNo,true);
	if(no < 0)
		msg->data.arg1 = no;
	else {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->data.arg1 = ERR_NO_MNTPNT;
		else
			msg->data.arg1 = inst->fs->stat(inst->handle,no,info);
	}
	send(fd,MSG_FS_STAT_RESP,msg,sizeof(msg->data));
}

static void cmds_istat(int fd,sMsg *msg) {
	inode_t ino = (inode_t)msg->args.arg1;
	dev_t devNo = (dev_t)msg->args.arg2;
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	sFSInst *inst = mount_get(devNo);
	if(inst == NULL)
		msg->data.arg1 = ERR_NO_MNTPNT;
	else
		msg->data.arg1 = inst->fs->stat(inst->handle,ino,info);
	send(fd,MSG_FS_STAT_RESP,msg,sizeof(msg->data));
}

static void cmds_chmod(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	mode_t mode = msg->str.arg1;
	dev_t devNo = rootDev;
	inode_t ino;
	sFSInst *inst;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->chmod == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else
			msg->args.arg1 = inst->fs->chmod(inst->handle,&u,ino,mode);
	}
	send(fd,MSG_FS_CHMOD_RESP,msg,sizeof(msg->args));
}

static void cmds_chown(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	uid_t uid = (uid_t)(msg->str.arg1 >> 16);
	gid_t gid = (gid_t)(msg->str.arg1 & 0xFFFF);
	dev_t devNo = rootDev;
	inode_t ino;
	sFSInst *inst;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->chown == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else
			msg->args.arg1 = inst->fs->chown(inst->handle,&u,ino,uid,gid);
	}
	send(fd,MSG_FS_CHOWN_RESP,msg,sizeof(msg->args));
}

static void cmds_read(int fd,sMsg *msg) {
	inode_t ino = (inode_t)msg->args.arg1;
	dev_t devNo = (dev_t)msg->args.arg2;
	uint offset = msg->args.arg3;
	size_t count = msg->args.arg4;
	sFSInst *inst = mount_get(devNo);
	void *buffer = NULL;
	if(inst == NULL)
		msg->args.arg1 = ERR_NO_MNTPNT;
	else if(inst->fs->read == NULL)
		msg->args.arg1 = ERR_UNSUPPORTED_OP;
	else {
		buffer = malloc(count);
		if(buffer == NULL)
			msg->args.arg1 = ERR_NOT_ENOUGH_MEM;
		else
			msg->args.arg1 = inst->fs->read(inst->handle,ino,buffer,offset,count);
	}
	send(fd,MSG_FS_READ_RESP,msg,sizeof(msg->args));
	if(buffer) {
		if(msg->args.arg1 > 0)
			send(fd,MSG_FS_READ_RESP,buffer,count);
		free(buffer);
	}
}

static void cmds_write(int fd,sMsg *msg,void *data) {
	inode_t ino = (inode_t)msg->args.arg1;
	dev_t devNo = (dev_t)msg->args.arg2;
	uint offset = msg->args.arg3;
	size_t count = msg->args.arg4;
	sFSInst *inst = mount_get(devNo);
	if(inst == NULL)
		msg->args.arg1 = ERR_NO_MNTPNT;
	else if(inst->fs->write == NULL)
		msg->args.arg1 = ERR_UNSUPPORTED_OP;
	/* write to file */
	else
		msg->args.arg1 = inst->fs->write(inst->handle,ino,data,offset,count);
	/* send response */
	send(fd,MSG_FS_WRITE_RESP,msg,sizeof(msg->args));
}

static void cmds_link(int fd,sMsg *msg) {
	char *oldPath = msg->str.s1;
	char *newPath = msg->str.s2;
	dev_t oldDev = rootDev,newDev = rootDev;
	inode_t dirIno,dstIno;
	sFSInst *inst;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	dstIno = root->fs->resPath(root->handle,&u,oldPath,IO_READ,&oldDev,true);
	inst = mount_get(oldDev);
	if(dstIno < 0)
		msg->args.arg1 = dstIno;
	else if(inst == NULL)
		msg->args.arg1 = ERR_NO_MNTPNT;
	else {
		/* split path and name */
		char *name,backup;
		size_t len = strlen(newPath);
		if(newPath[len - 1] == '/')
			newPath[len - 1] = '\0';
		name = strrchr(newPath,'/') + 1;
		backup = *name;
		dirname(newPath);

		dirIno = root->fs->resPath(root->handle,&u,newPath,IO_READ,&newDev,true);
		if(dirIno < 0)
			msg->args.arg1 = dirIno;
		else if(newDev != oldDev)
			msg->args.arg1 = ERR_LINK_DEVICE;
		else if(inst->fs->link == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->link(inst->handle,&u,dstIno,dirIno,name);
		}
	}
	send(fd,MSG_FS_LINK_RESP,msg,sizeof(msg->args));
}

static void cmds_unlink(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name;
	dev_t devNo = rootDev;
	inode_t dirIno;
	sFSInst *inst;
	char backup;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	dirIno = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(dirIno < 0)
		msg->args.arg1 = dirIno;
	else {
		/* split path and name */
		size_t len = strlen(path);
		if(path[len - 1] == '/')
			path[len - 1] = '\0';
		name = strrchr(path,'/') + 1;
		backup = *name;
		dirname(path);

		/* get directory */
		dirIno = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
		vassert(dirIno >= 0,"Subdir found, but parent not!?");
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->unlink == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->unlink(inst->handle,&u,dirIno,name);
		}
	}
	send(fd,MSG_FS_UNLINK_RESP,msg,sizeof(msg->args));
}

static void cmds_mkdir(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name,backup;
	inode_t dirIno;
	size_t len;
	dev_t devNo = rootDev;
	sFSInst *inst;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;

	/* split path and name */
	len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	name = strrchr(path,'/') + 1;
	backup = *name;
	dirname(path);

	dirIno = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(dirIno < 0)
		msg->args.arg1 = dirIno;
	else {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->mkdir == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->mkdir(inst->handle,&u,dirIno,name);
		}
	}
	send(fd,MSG_FS_MKDIR_RESP,msg,sizeof(msg->args));
}

static void cmds_rmdir(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name,backup;
	inode_t dirIno;
	size_t len;
	dev_t devNo = rootDev;
	sFSInst *inst;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;

	/* split path and name */
	len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	name = strrchr(path,'/') + 1;
	backup = *name;
	dirname(path);

	dirIno = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(dirIno < 0)
		msg->args.arg1 = dirIno;
	else {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->rmdir == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->rmdir(inst->handle,&u,dirIno,name);
		}
	}
	send(fd,MSG_FS_RMDIR_RESP,msg,sizeof(msg->args));
}

static void cmds_mount(int fd,sMsg *msg) {
	char *device = msg->str.s1;
	char *path = msg->str.s2;
	uint type = msg->str.arg1;
	dev_t devNo = rootDev;
	inode_t ino;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,true);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		sFSInst *inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else {
			sFileInfo info;
			int res = inst->fs->stat(inst->handle,ino,&info);
			if(res < 0)
				msg->args.arg1 = res;
			else if(!MODE_IS_DIR(info.mode))
				msg->args.arg1 = ERR_NO_DIRECTORY;
			else {
				int pnt = mount_addMnt(devNo,ino,device,type);
				msg->args.arg1 = pnt < 0 ? pnt : 0;
			}
		}
	}
	send(fd,MSG_FS_MOUNT_RESP,msg,sizeof(msg->args));
}

static void cmds_unmount(int fd,sMsg *msg) {
	char *path = msg->str.s1;
	dev_t devNo = rootDev;
	inode_t ino;
	sFSUser u;
	u.uid = msg->str.arg2;
	u.gid = msg->str.arg3;
	u.pid = msg->str.arg4;
	ino = root->fs->resPath(root->handle,&u,path,IO_READ,&devNo,false);
	if(ino < 0)
		msg->args.arg1 = ino;
	else
		msg->args.arg1 = mount_remMnt(devNo,ino);
	send(fd,MSG_FS_UNMOUNT_RESP,msg,sizeof(msg->args));
}

static void cmds_sync(int fd,sMsg *msg) {
	UNUSED(fd);
	UNUSED(msg);
	size_t i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		sFSInst *inst = mount_get(i);
		if(inst && inst->fs->sync != NULL)
			inst->fs->sync(inst->handle);
	}
}

static void cmds_close(int fd,sMsg *msg) {
	UNUSED(fd);
	inode_t ino = msg->args.arg1;
	dev_t devNo = msg->args.arg2;
	sFSInst *inst = mount_get(devNo);
	if(inst != NULL)
		inst->fs->close(inst->handle,ino);
}
