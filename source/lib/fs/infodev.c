/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/thread.h>
#include <fs/infodev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

static void sigUsr1(A_UNUSED int sig);
static void getFSInfo(FILE *str);

static volatile bool run = true;
static sFileSystem *fs;
static const char *path;

static int infodev_handler(A_UNUSED void *arg) {
	char devpath[MAX_PATH_LEN];
	sMsg msg;
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to announce USR1-signal-handler");

	char *devname = strrchr(path,'/');
	if(!devname)
		error("Invalid device name '%s'",path);
	snprintf(devpath,sizeof(devpath),"/system/fs/%s",devname);

	int id = createdev(devpath,0444,DEV_TYPE_FILE,DEV_READ | DEV_CLOSE);
	if(id < 0)
		error("Unable to create file %s",devpath);

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
					handleFileRead(fd,&msg,getFSInfo);
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

int infodev_start(const char *_path,sFileSystem *_fs) {
	path = _path;
	fs = _fs;
	int res;
	if((res = startthread(infodev_handler,NULL)) < 0)
		return res;
	return 0;
}

void infodev_shutdown(void) {
	if(kill(getpid(),SIG_USR1) < 0)
		printe("Unable to send signal to me");
}

static void sigUsr1(A_UNUSED int sig) {
	run = false;
}

static void getFSInfo(FILE *str) {
	fs->print(str,fs->handle);
}
