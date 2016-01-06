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

#pragma once

#include <esc/ipc/filedev.h>
#include <esc/vthrow.h>
#include <fs/filesystem.h>
#include <sys/common.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <stdlib.h>
#include <signal.h>

namespace fs {

template<class F>
class InfoDevice {
public:
	explicit InfoDevice(const char *path,FileSystem<F> *fs) : _tid(), _path(path), _fs(fs) {
		int res;
		if((res = startthread(thread,this)) < 0)
			VTHROWE("startthread",res);
		_tid = res;
	}
	~InfoDevice() {
		if(kill(getpid(),SIGUSR1) < 0)
			printe("Unable to send signal to me");
		IGNSIGS(join(_tid));
	}

	const char *path() const {
		return _path;
	}
	FileSystem<F> *fs() {
		return _fs;
	}

private:
	class FSFileDevice : public esc::FileDevice {
	public:
		explicit FSFileDevice(FileSystem<F> *fs,const char *path,mode_t mode)
			: esc::FileDevice(path,mode), _fs(fs) {
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
		FileSystem<F> *_fs;
	};

	static void sigUsr1(int) {
		if(dev)
			dev->stop();
	}

	static int thread(void *arg) {
		char devpath[MAX_PATH_LEN];
		InfoDevice<F> *idev = static_cast<InfoDevice<F>*>(arg);

		if(signal(SIGUSR1,sigUsr1) == SIG_ERR)
			error("Unable to announce USR1-signal-handler");

		char *devname = strrchr(idev->path(),'/');
		if(!devname)
			error("Invalid device name '%s'",idev->path());
		snprintf(devpath,sizeof(devpath),"/sys/fs/%s",devname);

		try {
			dev = new FSFileDevice(idev->fs(),devpath,0444);
			dev->loop();
		}
		catch(const std::exception &e) {
			printe("Warning: starting info device failed: %s",e.what());
		}
		return 0;
	}

	tid_t _tid;
	const char *_path;
	FileSystem<F> *_fs;
	static FSFileDevice *dev;
};

template<class F>
typename InfoDevice<F>::FSFileDevice *InfoDevice<F>::dev;

}
