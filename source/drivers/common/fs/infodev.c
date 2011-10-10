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
#include <esc/driver.h>
#include <esc/messages.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "mount.h"
#include "infodev.h"
#include "fslist.h"

typedef struct {
	const char *path;
	fGetData getData;
} sFSInfo;

static void sigUsr1(A_UNUSED int sig);
static sFSInfo *getById(int id);
static void getMounts(FILE *str);
static void getFSInsts(FILE *str);

static sFSInfo infos[] = {
	{"/system/mounts",getMounts},
	{"/system/filesystems",getFSInsts},
};
static int ids[ARRAY_SIZE(infos)];
static volatile bool run = true;

int infodev_thread(A_UNUSED void *arg) {
	sMsg msg;
	int id;
	msgid_t mid;
	size_t i;
	if(setSigHandler(SIG_USR1,sigUsr1) < 0)
		error("Unable to announce USR1-signal-handler");

	for(i = 0; i < ARRAY_SIZE(infos); i++) {
		ids[i] = createdev(infos[i].path,DEV_TYPE_FILE,DEV_READ);
		if(ids[i] < 0)
			error("Unable to create file %s",infos[i].path);
		if(chmod(infos[i].path,0644) < 0)
			error("Unable to chmod %s",infos[i].path);
	}

	while(run) {
		int fd = getWork(ids,ARRAY_SIZE(ids),&id,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("[FSINFO] Unable to get work");
		}
		else {
			sFSInfo *info = getById(id);
			if(info == NULL)
				printe("[FSINFO] Invalid device-id %d",id);
			else {
				switch(mid) {
					case MSG_DEV_READ:
						handleFileRead(fd,&msg,info->getData);
						break;

					default:
						msg.args.arg1 = -ENOTSUP;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
						break;
				}
			}
			close(fd);
		}
	}

	for(i = 0; i < ARRAY_SIZE(infos); i++)
		close(ids[i]);
	return 0;
}

void infodev_shutdown(void) {
	if(sendSignalTo(getpid(),SIG_USR1) < 0)
		printe("[FS] Unable to send signal to me");
}

static void sigUsr1(A_UNUSED int sig) {
	run = false;
}

static sFSInfo *getById(int id) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(ids); i++) {
		if(ids[i] == id)
			return infos + i;
	}
	return NULL;
}

static void getMounts(FILE *str) {
	size_t i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		const sMountPoint *mnt = mount_getByIndex(i);
		if(mnt) {
			const char *fs = fslist_getName(mnt->mnt->fs->type);
			fprintf(str,"%s on %s type %s\n",mnt->mnt->device,mnt->path,fs);
		}
	}
}

static void getFSInsts(FILE *str) {
	size_t i,j;
	for(i = 0; ;i++) {
		const char *fs;
		const sFSInst *inst = mount_getFSInst(i);
		if(!inst)
			break;

		fs = fslist_getName(inst->fs->type);
		fprintf(str,"%s on %s:\n",fs,inst->device);
		fprintf(str,"\tRefs: %u\n",inst->refs);
		fprintf(str,"\tMounted at: ");
		for(j = 0; j < MOUNT_TABLE_SIZE; j++) {
			const sMountPoint *mnt = mount_getByIndex(j);
			if(mnt && mnt->mnt == inst)
				fprintf(str,"'%s' ",mnt->path);
		}
		fprintf(str,"\n");
		inst->fs->print(str,inst->handle);
		fprintf(str,"\n");
	}
}
