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
#include <sys/vfs/openfile.h>
#include <sys/vfs/node.h>
#include <sys/col/slist.h>
#include <esc/fsinterface.h>

class Proc;

class VFSFS {
	VFSFS() = delete;

public:
	/**
	 * Retrieves information about the given (real!) path
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path in the real filesystem
	 * @param info should be filled
	 * @return 0 on success
	 */
	static int stat(pid_t pid,OpenFile *fsFile,const char *path,sFileInfo *info);

	/**
	 * Changes the permissions of the file denoted by <path>
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path
	 * @param mode the new mode
	 * @return 0 on success
	 */
	static int chmod(pid_t pid,OpenFile *fsFile,const char *path,mode_t mode);

	/**
	 * Changes the owner and group of the file denoted by <path>.
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path
	 * @param uid the user-id
	 * @param gid the group-id
	 * @return 0 on success
	 */
	static int chown(pid_t pid,OpenFile *fsFile,const char *path,uid_t uid,gid_t gid);

	/**
	 * Creates a hardlink at <newPath> which points to <oldPath>
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param oldPath the link-target
	 * @param newPath the link-path
	 * @return 0 on success
	 */
	static int link(pid_t pid,OpenFile *fsFile,const char *oldPath,const char *newPath);

	/**
	 * Unlinks the given path. That means, the directory-entry will be removed and if there are no
	 * more references to the inode, it will be removed.
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path
	 * @return 0 on success
	 */
	static int unlink(pid_t pid,OpenFile *fsFile,const char *path);

	/**
	 * Creates the given directory. Expects that all except the last path-component exist.
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path
	 * @return 0 on success
	 */
	static int mkdir(pid_t pid,OpenFile *fsFile,const char *path);

	/**
	 * Removes the given directory. Expects that the directory is empty (except '.' and '..')
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param path the path
	 * @return 0 on success
	 */
	static int rmdir(pid_t pid,OpenFile *fsFile,const char *path);
};
