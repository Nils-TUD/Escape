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
#include <fs/common.h>
#include <errno.h>
#include <stdio.h>

/**
 * The base-class for all filesystems
 */
class FileSystem {
public:
	explicit FileSystem() {
	}
	virtual ~FileSystem() {
	}

	virtual ino_t open(FSUser *u,ino_t ino,uint flags) = 0;

	virtual void close(ino_t ino) = 0;

	virtual ino_t resolve(FSUser *u,const char *path,uint flags) = 0;

	virtual int stat(ino_t ino,struct stat *info) = 0;

	virtual ssize_t read(ino_t,void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual ssize_t write(ino_t,const void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual int link(FSUser *,ino_t,ino_t,const char *) {
		return -ENOTSUP;
	}
	virtual int unlink(FSUser *,ino_t,const char *) {
		return -ENOTSUP;
	}
	virtual int mkdir(FSUser *,ino_t,const char *,mode_t) {
		return -ENOTSUP;
	}
	virtual int rmdir(FSUser *,ino_t,const char *) {
		return -ENOTSUP;
	}
	virtual int chmod(FSUser *,ino_t,mode_t) {
		return -ENOTSUP;
	}
	virtual int chown(FSUser *,ino_t,uid_t,gid_t) {
		return -ENOTSUP;
	}
	virtual void sync() {
	}

	virtual void print(FILE *f) = 0;
};
