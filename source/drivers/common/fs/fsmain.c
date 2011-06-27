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
#include "cmds.h"

#define FS_NAME_LEN		12

typedef struct {
	uint type;
	char name[FS_NAME_LEN];
	sFileSystem *(*fGetFS)(void);
} sFSType;

static void sigTermHndl(int sig);
static void shutdown(void);

static volatile bool run = true;
static sFSType types[] = {
	{FS_TYPE_EXT2,		"ext2",		ext2_getFS},
	{FS_TYPE_ISO9660,	"iso9660",	iso_getFS},
};
static sMsg msg;

int main(int argc,char *argv[]) {
	size_t i;
	int id;

	if(argc < 4) {
		printe("Usage: %s <wait> <driverPath> <fsType>",argv[0]);
		return EXIT_FAILURE;
	}

	if(setSigHandler(SIG_TERM,sigTermHndl) < 0)
		error("Unable to set signal-handler for SIG_TERM");

	tpool_init();
	mount_init();

	/* add filesystems */
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		sFileSystem *fs = types[i].fGetFS();
		if(!fs)
			error("Unable to get %s-filesystem",types[i].name);
		if(mount_addFS(fs) != 0)
			error("Unable to add %s-filesystem",types[i].name);
		printf("[FS] Loaded %s-driver\n",types[i].name);
	}

	/* create root-fs */
	uint fstype = 0;
	for(i = 0; i < ARRAY_SIZE(types); i++) {
		if(strcmp(types[i].name,argv[3]) == 0) {
			fstype = types[i].type;
			break;
		}
	}

	dev_t rootDev = mount_addMnt(ROOT_MNT_DEV,ROOT_MNT_INO,argv[2],fstype);
	if(rootDev < 0)
		error("Unable to add root mount-point");
	sFSInst *root = mount_get(rootDev);
	if(root == NULL)
		error("Unable to get root mount-point");
	cmds_setRoot(rootDev,root);
	printf("[FS] Mounted '%s' with fs '%s' at '/'\n",root->driver,argv[3]);
	fflush(stdout);

	/* register driver */
	id = regDriver("fs",DRV_FS);
	if(id < 0)
		error("Unable to register driver 'fs'");

	/* set permissions to zero; the kernel will open the file to us and won't check them */
	if(chmod("/dev/fs",0) < 0)
		error("Unable to set permissions for /dev/fs");

	msgid_t mid;
	while(true) {
		int fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),!run ? GW_NOBLOCK : 0);
		if(fd < 0) {
			if(fd != ERR_INTERRUPTED) {
				/* no requests anymore and we should shutdown? */
				if(!run)
					break;
				printe("[FS] Unable to get work");
			}
		}
		else {
			void *data = NULL;
			if(mid == MSG_FS_WRITE) {
				data = malloc(msg.args.arg4);
				if(!data || RETRY(receive(fd,NULL,data,msg.args.arg4)) < 0) {
					printf("[FS] Illegal request\n");
					close(fd);
					continue;
				}
			}

			if(!cmds_execute(mid,fd,&msg,data)) {
				msg.args.arg1 = ERR_UNSUPPORTED_OP;
				send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				close(fd);
			}
		}
	}

	/* clean up */
	tpool_shutdown();
	shutdown();
	close(id);

	return EXIT_SUCCESS;
}

static void sigTermHndl(int sig) {
	UNUSED(sig);
	run = false;
}

static void shutdown(void) {
	size_t i;
	for(i = 0; i < MOUNT_TABLE_SIZE; i++) {
		sFSInst *inst = mount_get(i);
		if(inst && inst->fs->sync != NULL)
			inst->fs->sync(inst->handle);
	}
	exit(EXIT_SUCCESS);
}
