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
#include <sys/stat.h>
#include <fs/infodev.h>
#include <ipc/filedev.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>

class FSFileDevice : public ipc::FileDevice {
public:
	explicit FSFileDevice(FileSystem *fs,const char *path,mode_t mode)
		: ipc::FileDevice(path,mode), _fs(fs) {
	}

	virtual std::string handleRead() {
		std::string res;
		FILE *str = fopendyn();
		if(str) {
			_fs->print(str);
			char *content = fgetbuf(str,NULL);
			res = std::string(content);
			fclose(str);
		}
		return res;
	}

private:
	FileSystem *_fs;
};

static FSFileDevice *dev;

static void sigUsr1(int) {
	dev->stop();
}

int InfoDevice::thread(void *arg) {
	char devpath[MAX_PATH_LEN];
	InfoDevice *idev = static_cast<InfoDevice*>(arg);

	if(signal(SIGUSR1,sigUsr1) == SIG_ERR)
		error("Unable to announce USR1-signal-handler");

	char *devname = strrchr(idev->path(),'/');
	if(!devname)
		error("Invalid device name '%s'",idev->path());
	snprintf(devpath,sizeof(devpath),"/sys/fs/%s",devname);

	dev = new FSFileDevice(idev->fs(),devpath,0444);
	dev->loop();
	return 0;
}
