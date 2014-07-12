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

#include <sys/common.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <fs/filesystem.h>
#include <vthrow.h>
#include <signal.h>

class InfoDevice {
public:
	explicit InfoDevice(const char *path,FileSystem *fs) : _tid(), _path(path), _fs(fs) {
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
	FileSystem *fs() {
		return _fs;
	}

private:
	static int thread(void*);

	tid_t _tid;
	const char *_path;
	FileSystem *_fs;
};
