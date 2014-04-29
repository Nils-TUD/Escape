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

#include <esc/common.h>
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

	virtual inode_t open(FSUser *u,inode_t ino,uint flags) = 0;

	virtual void close(inode_t ino) = 0;

	virtual inode_t resolve(FSUser *u,const char *path,uint flags) = 0;

	virtual int stat(inode_t ino,sFileInfo *info) = 0;

	virtual ssize_t read(inode_t,void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual ssize_t write(inode_t,const void *,off_t,size_t) {
		return -ENOTSUP;
	}
	virtual int link(FSUser *,inode_t,inode_t,const char *) {
		return -ENOTSUP;
	}
	virtual int unlink(FSUser *,inode_t,const char *) {
		return -ENOTSUP;
	}
	virtual int mkdir(FSUser *,inode_t,const char *) {
		return -ENOTSUP;
	}
	virtual int rmdir(FSUser *,inode_t,const char *) {
		return -ENOTSUP;
	}
	virtual int chmod(FSUser *,inode_t,mode_t) {
		return -ENOTSUP;
	}
	virtual int chown(FSUser *,inode_t,uid_t,gid_t) {
		return -ENOTSUP;
	}
	virtual void sync() {
	}

	virtual void print(FILE *f) = 0;
};
