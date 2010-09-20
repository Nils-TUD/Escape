/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/io.h>
#include <esc/fsinterface.h>
#include <esc/messages.h>
#include <esc/driver.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <esc/dir.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

#include "ext2/ext2.h"
#include "iso9660/iso9660.h"
#include "mount.h"
#include "threadpool.h"

#define FS_NAME_LEN		12

static tDevNo rootDev;
static sFSInst *root;

typedef void (*fFSCmd)(tFD fd,sMsg *msg);

typedef struct {
	u16 type;
	char name[FS_NAME_LEN];
} sFSType;

static void shutdown(void);
static void cmdOpen(tFD fd,sMsg *msg);
static void cmdRead(tFD fd,sMsg *msg);
static void cmdWrite(tFD fd,sMsg *msg);
static void cmdClose(tFD fd,sMsg *msg);
static void cmdStat(tFD fd,sMsg *msg);
static void cmdSync(tFD fd,sMsg *msg);
static void cmdLink(tFD fd,sMsg *msg);
static void cmdUnlink(tFD fd,sMsg *msg);
static void cmdMkdir(tFD fd,sMsg *msg);
static void cmdRmdir(tFD fd,sMsg *msg);
static void cmdMount(tFD fd,sMsg *msg);
static void cmdUnmount(tFD fd,sMsg *msg);
static void cmdIstat(tFD fd,sMsg *msg);

static bool run = true;

static fFSCmd commands[] = {
	/* MSG_FS_OPEN */		cmdOpen,
	/* MSG_FS_READ */		cmdRead,
	/* MSG_FS_WRITE */		cmdWrite,
	/* MSG_FS_CLOSE */		cmdClose,
	/* MSG_FS_STAT */		cmdStat,
	/* MSG_FS_SYNC */		cmdSync,
	/* MSG_FS_LINK */		cmdLink,
	/* MSG_FS_UNLINK */		cmdUnlink,
	/* MSG_FS_MKDIR */		cmdMkdir,
	/* MSG_FS_RMDIR */		cmdRmdir,
	/* MSG_FS_MOUNT */		cmdMount,
	/* MSG_FS_UNMOUNT */	cmdUnmount,
	/* MSG_FS_ISTAT */		cmdIstat,
};

static void sigTermHndl(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	run = false;
}

int main(int argc,char *argv[]) {
	static sFSType types[] = {
		{FS_TYPE_EXT2,"ext2"},
		{FS_TYPE_ISO9660,"iso9660"},
	};
	static sMsg msg;
	tFD fd;
	tMsgId mid;
	u32 i,fstype;
	tDrvId id;
	sFileSystem *fs;

	if(argc < 3) {
		printe("Usage: %s <driverPath> <fsType>",argv[0]);
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_TERM,sigTermHndl) < 0)
		error("Unable to set signal-handler for SIG_TERM");

	tpool_init();
	mount_init();

	/* add ext2 */
	fs = ext2_getFS();
	if(!fs)
		error("Unable to get ext2-filesystem");
	if(mount_addFS(fs) != 0)
		error("Unable to add ext2-filesystem");
	printf("[FS] Loaded ext2-driver\n");

	/* add iso9660 */
	fs = iso_getFS();
	if(!fs)
		error("Unable to get iso9660-filesystem");
	if(mount_addFS(fs) != 0)
		error("Unable to add iso9660-filesystem");
	printf("[FS] Loaded iso9660-driver\n");

	/* create root-fs */
	fstype = 0;
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		if(strcmp(types[i].name,argv[2]) == 0) {
			fstype = types[i].type;
			break;
		}
	}

	rootDev = mount_addMnt(ROOT_MNT_DEV,ROOT_MNT_INO,argv[1],fstype);
	if(rootDev < 0)
		error("Unable to add root mount-point");
	root = mount_get(rootDev);
	if(root == NULL)
		error("Unable to get root mount-point");
	printf("[FS] Mounted '%s' with fs '%s' at '/'\n",root->driver,argv[2]);
	fflush(stdout);

	/* register driver */
	id = regDriver("fs",DRV_FS);
	if(id < 0)
		error("Unable to register driver 'fs'");

	while(run) {
		fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED)
				printe("[FS] Unable to get work");
		}
		else {
			if(mid < MSG_FS_OPEN || mid > MSG_FS_ISTAT)
				printf("[FS] Illegal command %d\n",mid);
			else {
				void *data = NULL;
				while(!tpool_hasFreeSlot())
					wait(EV_USER2);
				if(mid == MSG_FS_WRITE) {
					data = malloc(msg.args.arg4);
					if(!data || RETRY(receive(fd,NULL,data,msg.args.arg4)) < 0) {
						printf("[FS] Illegal request\n");
						close(fd);
						continue;
					}
				}
				if(!tpool_addRequest(commands[mid - MSG_FS_OPEN],fd,&msg,sizeof(msg),data))
					printf("[FS] Not enough mem for request %d\n",mid);
				/*commands[mid - MSG_FS_OPEN](fd,&msg);
				close(fd);*/
			}
		}
	}

	/* clean up */
	shutdown();
	unregDriver(id);

	return EXIT_SUCCESS;
}

static void shutdown(void) {
	u32 i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		sFSInst *inst = mount_get(i);
		if(inst && inst->fs->sync != NULL)
			inst->fs->sync(inst->handle);
	}
	exit(EXIT_SUCCESS);
}

static void cmdOpen(tFD fd,sMsg *msg) {
	tDevNo devNo = rootDev;
	u8 flags = (u8)msg->args.arg1;
	sFSInst *inst;
	tInodeNo no = root->fs->resPath(root->handle,msg->str.s1,flags,&devNo,true);
	if(no >= 0) {
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else
			msg->args.arg1 = inst->fs->open(inst->handle,no,flags);
	}
	else
		msg->args.arg1 = ERR_PATH_NOT_FOUND;
	msg->args.arg2 = devNo;
	send(fd,MSG_FS_OPEN_RESP,msg,sizeof(msg->args));
}

static void cmdStat(tFD fd,sMsg *msg) {
	tDevNo devNo = rootDev;
	sFSInst *inst;
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	tInodeNo no = root->fs->resPath(root->handle,msg->str.s1,IO_READ,&devNo,true);
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

static void cmdIstat(tFD fd,sMsg *msg) {
	tInodeNo ino = (tInodeNo)msg->args.arg1;
	tDevNo devNo = (tDevNo)msg->args.arg2;
	sFileInfo *info = (sFileInfo*)&(msg->data.d);
	sFSInst *inst = mount_get(devNo);
	if(inst == NULL)
		msg->data.arg1 = ERR_NO_MNTPNT;
	else
		msg->data.arg1 = inst->fs->stat(inst->handle,ino,info);
	send(fd,MSG_FS_STAT_RESP,msg,sizeof(msg->data));
}

static void cmdRead(tFD fd,sMsg *msg) {
	tInodeNo ino = (tInodeNo)msg->args.arg1;
	tDevNo devNo = (tDevNo)msg->args.arg2;
	u32 offset = msg->args.arg3;
	u32 count = msg->args.arg4;
	sFSInst *inst = mount_get(devNo);
	u8 *buffer = NULL;
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

	/* read ahead
	if(count > 0)
		ext2_file_read(&ext2,data.inodeNo,NULL,data.offset + count,data.count);*/
}

static void cmdWrite(tFD fd,sMsg *msg) {
	tMsgId mid;
	tInodeNo ino = (tInodeNo)msg->args.arg1;
	tDevNo devNo = (tDevNo)msg->args.arg2;
	u32 offset = msg->args.arg3;
	u32 count = msg->args.arg4;
	sFSInst *inst = mount_get(devNo);
	u8 *buffer;
	if(inst == NULL)
		msg->args.arg1 = ERR_NO_MNTPNT;
	else if(inst->fs->write == NULL)
		msg->args.arg1 = ERR_UNSUPPORTED_OP;
	else {
		/* write to file */
		msg->args.arg1 = 0;
		buffer = malloc(count);
		if(buffer) {
			if(RETRY(receive(fd,&mid,buffer,count)) >= 0)
				msg->args.arg1 = inst->fs->write(inst->handle,ino,buffer,offset,count);
			free(buffer);
		}
	}
	/* send response */
	send(fd,MSG_FS_WRITE_RESP,msg,sizeof(msg->args));
}

static void cmdLink(tFD fd,sMsg *msg) {
	char *oldPath = msg->str.s1;
	char *newPath = msg->str.s2;
	tDevNo oldDev = rootDev,newDev = rootDev;
	sFSInst *inst;
	tInodeNo dirIno,dstIno = root->fs->resPath(root->handle,oldPath,IO_READ,&oldDev,true);
	inst = mount_get(oldDev);
	if(dstIno < 0)
		msg->args.arg1 = dstIno;
	else if(inst == NULL)
		msg->args.arg1 = ERR_NO_MNTPNT;
	else {
		/* split path and name */
		char *name,backup;
		u32 len = strlen(newPath);
		if(newPath[len - 1] == '/')
			newPath[len - 1] = '\0';
		name = strrchr(newPath,'/') + 1;
		backup = *name;
		dirname(newPath);

		dirIno = root->fs->resPath(root->handle,newPath,IO_READ,&newDev,true);
		if(dirIno < 0)
			msg->args.arg1 = dirIno;
		else if(newDev != oldDev)
			msg->args.arg1 = ERR_LINK_DEVICE;
		else if(inst->fs->link == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->link(inst->handle,dstIno,dirIno,name);
		}
	}
	send(fd,MSG_FS_LINK_RESP,msg,sizeof(msg->args));
}

static void cmdUnlink(tFD fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name;
	tDevNo devNo = rootDev;
	tInodeNo dirIno;
	sFSInst *inst;
	char backup;
	dirIno = root->fs->resPath(root->handle,path,IO_READ,&devNo,true);
	if(dirIno < 0)
		msg->args.arg1 = dirIno;
	else {
		/* split path and name */
		u32 len = strlen(path);
		if(path[len - 1] == '/')
			path[len - 1] = '\0';
		name = strrchr(path,'/') + 1;
		backup = *name;
		dirname(path);

		/* get directory */
		dirIno = root->fs->resPath(root->handle,path,IO_READ,&devNo,true);
		vassert(dirIno >= 0,"Subdir found, but parent not!?");
		inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else if(inst->fs->unlink == NULL)
			msg->args.arg1 = ERR_UNSUPPORTED_OP;
		else {
			*name = backup;
			msg->args.arg1 = inst->fs->unlink(inst->handle,dirIno,name);
		}
	}
	send(fd,MSG_FS_UNLINK_RESP,msg,sizeof(msg->args));
}

static void cmdMkdir(tFD fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name,backup;
	tInodeNo dirIno;
	tDevNo devNo = rootDev;
	sFSInst *inst;

	/* split path and name */
	u32 len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	name = strrchr(path,'/') + 1;
	backup = *name;
	dirname(path);

	dirIno = root->fs->resPath(root->handle,path,IO_READ,&devNo,true);
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
			msg->args.arg1 = inst->fs->mkdir(inst->handle,dirIno,name);
		}
	}
	send(fd,MSG_FS_MKDIR_RESP,msg,sizeof(msg->args));
}

static void cmdRmdir(tFD fd,sMsg *msg) {
	char *path = msg->str.s1;
	char *name,backup;
	tInodeNo dirIno;
	tDevNo devNo = rootDev;
	sFSInst *inst;

	/* split path and name */
	u32 len = strlen(path);
	if(path[len - 1] == '/')
		path[len - 1] = '\0';
	name = strrchr(path,'/') + 1;
	backup = *name;
	dirname(path);

	dirIno = root->fs->resPath(root->handle,path,IO_READ,&devNo,true);
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
			msg->args.arg1 = inst->fs->rmdir(inst->handle,dirIno,name);
		}
	}
	send(fd,MSG_FS_RMDIR_RESP,msg,sizeof(msg->args));
}

static void cmdMount(tFD fd,sMsg *msg) {
	char *device = msg->str.s1;
	char *path = msg->str.s2;
	u16 type = (u16)msg->str.arg1;
	tDevNo devNo = rootDev;
	tInodeNo ino;
	ino = root->fs->resPath(root->handle,path,IO_READ,&devNo,true);
	if(ino < 0)
		msg->args.arg1 = ino;
	else {
		sFSInst *inst = mount_get(devNo);
		if(inst == NULL)
			msg->args.arg1 = ERR_NO_MNTPNT;
		else {
			sFileInfo info;
			s32 res = inst->fs->stat(inst->handle,ino,&info);
			if(res < 0)
				msg->args.arg1 = res;
			else if(!MODE_IS_DIR(info.mode))
				msg->args.arg1 = ERR_NO_DIRECTORY;
			else {
				s32 pnt = mount_addMnt(devNo,ino,device,type);
				msg->args.arg1 = pnt < 0 ? pnt : 0;
			}
		}
	}
	send(fd,MSG_FS_MOUNT_RESP,msg,sizeof(msg->args));
}

static void cmdUnmount(tFD fd,sMsg *msg) {
	char *path = msg->str.s1;
	tDevNo devNo = rootDev;
	tInodeNo ino;
	ino = root->fs->resPath(root->handle,path,IO_READ,&devNo,false);
	if(ino < 0)
		msg->args.arg1 = ino;
	else
		msg->args.arg1 = mount_remMnt(devNo,ino);
	send(fd,MSG_FS_UNMOUNT_RESP,msg,sizeof(msg->args));
}

static void cmdSync(tFD fd,sMsg *msg) {
	UNUSED(fd);
	UNUSED(msg);
	u32 i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		sFSInst *inst = mount_get(i);
		if(inst && inst->fs->sync != NULL)
			inst->fs->sync(inst->handle);
	}
}

static void cmdClose(tFD fd,sMsg *msg) {
	UNUSED(fd);
	tInodeNo ino = msg->args.arg1;
	tDevNo devNo = msg->args.arg2;
	sFSInst *inst = mount_get(devNo);
	if(inst != NULL)
		inst->fs->close(inst->handle,ino);
}
