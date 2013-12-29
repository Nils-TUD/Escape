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
	 * Opens the given path with given flags for given process
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param flags read / write
	 * @param path the path
	 * @param file will be set to the opened file
	 * @return 0 on success or the error-code
	 */
	static int openPath(pid_t pid,OpenFile *fsFile,uint flags,const char *path,OpenFile **file);

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
	 * Retrieves information about the given inode on given device
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param ino the inode-number
	 * @param info should be filled
	 * @return 0 on success
	 */
	static int istat(pid_t pid,OpenFile *fsFile,inode_t ino,sFileInfo *info);

	/**
	 * Reads from the given inode at <offset> <count> bytes into the given buffer
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param inodeNo the inode
	 * @param buffer the buffer to fill
	 * @param offset the offset in the data
	 * @param count the number of bytes to copy
	 * @return the number of read bytes
	 */
	static ssize_t read(pid_t pid,OpenFile *fsFile,inode_t inodeNo,void *buffer,
		off_t offset,size_t count);

	/**
	 * Writes to the given inode at <offset> <count> bytes from the given buffer
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param inodeNo the inode
	 * @param buffer the buffer
	 * @param offset the offset in the inode
	 * @param count the number of bytes to copy
	 * @return the number of written bytes
	 */
	static ssize_t write(pid_t pid,OpenFile *fsFile,inode_t inodeNo,const void *buffer,
		off_t offset,size_t count);

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

	/**
	 * Writes all dirty objects of the affected filesystem to disk
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @return 0 on success
	 */
	static int syncfs(pid_t pid,OpenFile *fsFile);

	/**
	 * Closes the given inode
	 *
	 * @param pid the process-id
	 * @param fsFile the channel to the fs instance
	 * @param inodeNo the inode
	 */
	static void close(pid_t pid,OpenFile *fsFile,inode_t inodeNo);

private:
	/* for istat() and stat() */
	static int doStat(pid_t pid,OpenFile *fsFile,const char *path,inode_t ino,sFileInfo *info);
	/* The request-handler for sending a path and receiving a result */
	static int pathReqHandler(pid_t pid,OpenFile *fsFile,const char *path1,const char *path2,
		uint arg1,uint cmd);
};
