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

#include <fs/common.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <utime.h>

namespace fs {

/**
 * The base-class for all filesystems
 */
template<class F>
class FileSystem {
public:
	explicit FileSystem() {
	}
	virtual ~FileSystem() {
	}

	virtual ino_t open(User *u,const char *path,ino_t root,uint flags,mode_t mode,int fd,F **) = 0;

	virtual void close(F *file) = 0;

	virtual int stat(F *file,struct ::stat *info) = 0;

	virtual ssize_t read(F *,void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual ssize_t write(F *,const void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual int link(User *,F *,F *,const char *) {
		return -ENOTSUP;
	}
	virtual int unlink(User *,F *,const char *) {
		return -ENOTSUP;
	}
	virtual int mkdir(User *,F *,const char *,mode_t) {
		return -ENOTSUP;
	}
	virtual int rmdir(User *,F *,const char *) {
		return -ENOTSUP;
	}
	virtual int rename(User *,F *,const char *,F *,const char *) {
		return -ENOTSUP;
	}
	virtual int chmod(User *,F *,mode_t) {
		return -ENOTSUP;
	}
	virtual int chown(User *,F *,uid_t,gid_t) {
		return -ENOTSUP;
	}
	virtual int utime(User *,F *,const struct utimbuf *) {
		return -ENOTSUP;
	}
	virtual int truncate(User *,F *,off_t) {
		return -ENOTSUP;
	}
	virtual void sync() {
	}

	virtual void print(FILE *f) = 0;
};

}
