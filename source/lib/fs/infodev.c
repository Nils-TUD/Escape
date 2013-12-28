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

#if 0

#include <esc/common.h>
#include <esc/driver.h>
#include <esc/messages.h>
#include <esc/thread.h>
#include <fs/infodev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

#include "mount.h"
#include "fslist.h"

typedef struct {
	const char *path;
	fGetData getData;
} sFSInfo;

static void sigUsr1(A_UNUSED int sig);
static void getMounts(FILE *str);
static void getFSInsts(FILE *str);

static sFSInfo infos[] = {
	{"/system/mounts",getMounts},
	{"/system/filesystems",getFSInsts},
};
static volatile bool run = true;

static int infodev_handler(void *arg) {
	sMsg msg;
	sFSInfo *info = (sFSInfo*)arg;
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to announce USR1-signal-handler");

	int id = createdev(info->path,0444,DEV_TYPE_FILE,DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("Unable to create file %s",info->path);

	while(run) {
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_DEV_READ:
					handleFileRead(fd,&msg,info->getData);
					break;

				case MSG_DEV_CLOSE:
					close(fd);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}
	close(id);
	return 0;
}

int infodev_thread(A_UNUSED void *arg) {
	if(startthread(infodev_handler, infos + 1) < 0)
		error("Unable to start thread");
	return infodev_handler(infos + 0);
}

void infodev_shutdown(void) {
	if(kill(getpid(),SIG_USR1) < 0)
		printe("Unable to send signal to me");
}

static void sigUsr1(A_UNUSED int sig) {
	run = false;
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

#endif
