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
#include <fs/threadpool.h>
#include <fs/infodev.h>
#include <fs/cmds.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

static void sigTermHndl(int sig);
static void shutdown(void);

static volatile bool run = true;
static sFileSystem *fs;

int fs_driverLoop(const char *name,const char *diskDev,const char *fsDev,sFileSystem *_fs) {
	size_t clientCount = 0;
	if(signal(SIG_TERM,sigTermHndl) == SIG_ERR)
		error("Unable to set signal-handler for SIG_TERM");

	fs = _fs;
	int err = 0;
	char *useddev;
	fs->handle = fs->init(diskDev,&useddev,&err);
	if(err < 0)
		error("Unable to init filesystem");
	printf("[%s] Mounted '%s'\n",name,useddev);
	fflush(stdout);

	if(infodev_start(fsDev,fs) < 0)
		error("Unable to start infodev-handler");
	tpool_init();

	/* register device (exec permission is enough) */
	int id = createdev(fsDev,0111,DEV_TYPE_FS,DEV_OPEN | DEV_CLOSE);
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
				data = malloc(msg.args.arg3);
				if(!data || IGNSIGS(receive(fd,NULL,data,msg.args.arg3)) < 0) {
					printe("Illegal request");
					close(fd);
					continue;
				}
			}
			else if(mid == MSG_DEV_OPEN) {
				clientCount++;
				msg.args.arg1 = 0;
				send(fd,MSG_DEV_OPEN_RESP,&msg,sizeof(msg.args));
				continue;
			}
			else if(mid == MSG_DEV_CLOSE) {
				close(fd);
				if(--clientCount == 0)
					break;
				continue;
			}

			if(!cmds_execute(fs,mid,fd,&msg,data)) {
				msg.args.arg1 = -ENOTSUP;
				send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
			}
		}
	}

	/* clean up */
	printf("Shutting down '%s'\n",fsDev);
	infodev_shutdown();
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
	if(fs->sync)
		fs->sync(fs->handle);
	exit(EXIT_SUCCESS);
}
