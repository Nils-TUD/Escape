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

#include "mount.h"
#include "infodev.h"
#include "fslist.h"

typedef void (*fGetData)(FILE *str);
typedef struct {
	const char *path;
	fGetData getData;
} sFSInfo;

static sFSInfo *getById(int id);
static void getMounts(FILE *str);
static void getFSInsts(FILE *str);

static sFSInfo infos[] = {
	{"/system/mounts",getMounts},
	{"/system/filesystems",getFSInsts},
};
static int ids[ARRAY_SIZE(infos)];

int infodev_thread(A_UNUSED void *arg) {
	size_t i;
	for(i = 0; i < ARRAY_SIZE(infos); i++) {
		ids[i] = createdev(infos[i].path,DEV_TYPE_FILE,DRV_READ);
		if(ids[i] < 0)
			error("Unable to create file %s",infos[i].path);
		if(chmod(infos[i].path,0644) < 0)
			error("Unable to chmod %s",infos[i].path);
	}

	while(1) {
		sMsg msg;
		int id;
		msgid_t mid;
		int fd = getWork(ids,ARRAY_SIZE(ids),&id,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[FSINFO] Unable to get work");
		else {
			sFSInfo *info = getById(id);
			if(info == NULL)
				printe("[FSINFO] Invalid device-id %d",id);
			else {
				switch(mid) {
					case MSG_DRV_READ: {
						size_t total;
						char *data;
						off_t offset = msg.args.arg1;
						size_t count = msg.args.arg2;
						FILE *str;
						if(offset + (off_t)count < offset)
							msg.args.arg1 = ERR_INVALID_ARGS;
						str = ascreate();
						if(str)
							info->getData(str);
						data = asget(str,&total);
						if(!data)
							msg.args.arg1 = ERR_NOT_ENOUGH_MEM;
						else {
							msg.args.arg1 = 0;
							if(offset >= (off_t)total)
								msg.args.arg1 = 0;
							else if(offset + count > total)
								msg.args.arg1 = total - offset;
							else
								msg.args.arg1 = count;
						}
						send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.args));
						if((long)msg.args.arg1 > 0)
							send(fd,MSG_DRV_READ_RESP,data + offset,msg.args.arg1);
						fclose(str);
					}
					break;

					default:
						msg.args.arg1 = ERR_UNSUPPORTED_OP;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
						break;
				}
			}
			close(fd);
		}
	}
	/* never reached */
	return 0;
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
			fprintf(str,"%s on %s type %s\n",mnt->mnt->driver,mnt->path,fs);
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
		fprintf(str,"%s on %s:\n",fs,inst->driver);
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
