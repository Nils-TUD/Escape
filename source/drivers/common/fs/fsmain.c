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
#include <esc/driver/init.h>
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
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "fslist.h"
#include "mount.h"
#include "threadpool.h"
#include "infodev.h"
#include "cmds.h"

static void sigTermHndl(int sig);
static void shutdown(void);

static volatile bool run = true;
static sMsg msg;

int main(int argc,char *argv[]) {
	int id;
	dev_t rootDev;
	sFSInst *root;
	msgid_t mid;
	int fstype;

	if(argc < 4) {
		printe("Usage: %s <wait> <devicePath> <fsType>",argv[0]);
		return EXIT_FAILURE;
	}

	if(signal(SIG_TERM,sigTermHndl) == SIG_ERR)
		error("Unable to set signal-handler for SIG_TERM");

	tpool_init();
	mount_init();
	fslist_init();
	if(startthread(infodev_thread,NULL) < 0)
		error("Unable to start infodev-thread");

	/* create root-fs */
	fstype = fslist_getType(argv[3]);
	if(fstype == -1)
		error("Unable to find filesystem '%s'",argv[3]);

	rootDev = mount_addMnt(ROOT_MNT_DEV,ROOT_MNT_INO,"/",argv[2],fstype);
	if(rootDev < 0)
		error("Unable to add root mount-point");
	root = mount_get(rootDev);
	if(root == NULL)
		error("Unable to get root mount-point");
	cmds_setRoot(rootDev,root);
	printf("[FS] Mounted '%s' with fs '%s' at '/'\n",root->device,argv[3]);
	fflush(stdout);

	/* register device */
	id = createdev("/dev/fs",DEV_TYPE_FS,0);
	if(id < 0)
		error("Unable to register device 'fs'");

	/* set permissions to zero; the kernel will open the file to us and won't check them */
	if(chmod("/dev/fs",0) < 0)
		error("Unable to set permissions for /dev/fs");

	while(true) {
		int fd = getwork(&id,1,NULL,&mid,&msg,sizeof(msg),!run ? GW_NOBLOCK : 0);
		if(fd < 0) {
			if(fd != -EINTR) {
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
				if(!data || IGNSIGS(receive(fd,NULL,data,msg.args.arg4)) < 0) {
					printf("[FS] Illegal request\n");
					close(fd);
					continue;
				}
			}

			if(!cmds_execute(mid,fd,&msg,data)) {
				msg.args.arg1 = -ENOTSUP;
				send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				close(fd);
			}
		}
	}

	/* clean up */
	infodev_shutdown();
	tpool_shutdown();
	join(0);
	shutdown();
	close(id);

	return EXIT_SUCCESS;
}

static void sigTermHndl(A_UNUSED int sig) {
	/* notify init that we're alive and promise to terminate as soon as possible */
	init_iamalive();
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
