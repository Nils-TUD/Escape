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
#include <ipc/filedev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

class FSFileDevice;

static int infodev_handler(A_UNUSED void *arg);
static void sigUsr1(A_UNUSED int sig);

static sFileSystem *fs;
static const char *fspath;
static FSFileDevice *dev;

class FSFileDevice : public ipc::FileDevice {
public:
	explicit FSFileDevice(const char *path,mode_t mode)
		: ipc::FileDevice(path,mode) {
	}

	virtual std::string handleRead() {
		FILE *str = ascreate();
		if(str) {
			fs->print(str,fs->handle);
			fclose(str);
			return asget(str,NULL);
		}
		return "";
	}
};

int infodev_start(const char *_path,sFileSystem *_fs) {
	fspath = _path;
	fs = _fs;
	int res;
	if((res = startthread(infodev_handler,NULL)) < 0)
		return res;
	return 0;
}

static int infodev_handler(A_UNUSED void *arg) {
	char devpath[MAX_PATH_LEN];
	if(signal(SIG_USR1,sigUsr1) == SIG_ERR)
		error("Unable to announce USR1-signal-handler");

	char *devname = strrchr(fspath,'/');
	if(!devname)
		error("Invalid device name '%s'",fspath);
	snprintf(devpath,sizeof(devpath),"/system/fs/%s",devname);

	dev = new FSFileDevice(devpath,0444);
	dev->loop();
	return 0;
}

static void sigUsr1(A_UNUSED int sig) {
	dev->stop();
}

void infodev_shutdown(void) {
	if(kill(getpid(),SIG_USR1) < 0)
		printe("Unable to send signal to me");
}
