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
#include <esc/fsinterface.h>
#include <esc/driver.h>
#include <fs/fsdev.h>
#include <fs/mount.h>
#include <fs/threadpool.h>
#include <fs/cmds.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void sigTermHndl(int sig);
static void shutdown(void);

static volatile bool run = true;

int fs_driverLoop(const char *diskDev,const char *fsDev,int type) {
	if(signal(SIG_TERM,sigTermHndl) == SIG_ERR)
		error("Unable to set signal-handler for SIG_TERM");

	tpool_init();

	/* create root-fs */
	dev_t rootDev = mount_addMnt(ROOT_MNT_DEV,ROOT_MNT_INO,"/",diskDev,type);
	if(rootDev < 0)
		error("Unable to add root mount-point");
	sFSInst *root = mount_get(rootDev);
	if(root == NULL)
		error("Unable to get root mount-point");
	cmds_setRoot(rootDev,root);

	/* register device (exec permission is enough) */
	int id = createdev(fsDev,0111,DEV_TYPE_FS,DEV_CLOSE);
	if(id < 0)
		error("Unable to register device 'fs'");

	while(true) {
		sMsg msg;
		msgid_t mid;
		int fd = getwork(id,&mid,&msg,sizeof(msg),!run ? GW_NOBLOCK : 0);
		if(fd < 0) {
			if(fd != -EINTR) {
				/* no requests anymore and we should shutdown? */
				if(!run)
					break;
				printe("Unable to get work");
			}
		}
		else {
			void *data = NULL;
			if(mid == MSG_FS_WRITE) {
				data = malloc(msg.args.arg4);
				if(!data || IGNSIGS(receive(fd,NULL,data,msg.args.arg4)) < 0) {
					printe("Illegal request");
					close(fd);
					continue;
				}
			}
			else if(mid == MSG_DEV_CLOSE) {
				close(fd);
				continue;
			}

			if(!cmds_execute(mid,fd,&msg,data)) {
				msg.args.arg1 = -ENOTSUP;
				send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
			}
		}
	}

	/* clean up */
	tpool_shutdown();
	shutdown();
	close(id);
	return 0;
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
