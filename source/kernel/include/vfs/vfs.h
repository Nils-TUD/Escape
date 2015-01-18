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

#include <vfs/node.h>
#include <common.h>

#define MAX_VFS_FILE_SIZE			(1024 * 1024)

#define DEV_OPEN					1
#define DEV_READ					2
#define DEV_WRITE					4
#define DEV_CLOSE					8
#define DEV_SHFILE					16
#define DEV_CANCEL					32
#define DEV_CANCELSIG				64
#define DEV_CREATSIBL				128
#define DEV_SIZE					256

#define DEV_TYPE_CHAR				0
#define DEV_TYPE_BLOCK				1
#define DEV_TYPE_SERVICE			2
#define DEV_TYPE_FS					3
#define DEV_TYPE_FILE				4

/* fcntl-commands */
#define F_GETFL						0
#define F_SETFL						1
#define F_GETACCESS					2
#define F_SEMUP						3
#define F_SEMDOWN					4
#define F_DISMSGS					5

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* getwork-flags */
#define GW_NOBLOCK					1

/* all flags that the user can use */
#define VFS_USER_FLAGS				(VFS_WRITE | VFS_READ | VFS_MSGS | VFS_CREATE | VFS_TRUNCATE | \
									 VFS_APPEND | VFS_NOBLOCK | VFS_LONELY | VFS_EXCL)

class Proc;
class VFSMS;

class VFS {
	VFS() = delete;

public:
	/**
	 * Initializes the virtual file system
	 */
	static void init();

	/**
	 * @return the directory of mountspaces
	 */
	static VFSNode *getMSDir() {
		return msNode;
	}

	/**
	 * Mounts the virtual filesystems into the given process. Should only be used by init.
	 *
	 * @param p the process
	 */
	static void mountAll(Proc *p);

	/**
	 * Clones the given mountspace into a new VFS node and assigns it to <p>.
	 *
	 * @param p the process
	 * @param src the mountspace to clone
	 * @param name the node name
	 * @return 0 on success
	 */
	static int cloneMS(Proc *p,const VFSMS *src,const char *name);

	/**
	 * Checks whether the process with given id has permission to use <n> with the given flags
	 *
	 * @param pid the process-id
	 * @param n the node
	 * @param flags the flags (VFS_*)
	 * @return 0 if successfull, negative if the process has no permission
	 */
	static int hasAccess(pid_t pid,const VFSNode *n,ushort flags);

	/**
	 * Opens the given path with given flags. That means it walks through the global
	 * file table and searches for a free entry or an entry for that file.
	 * Note that multiple processs may read from the same file simultaneously but NOT write!
	 *
	 * @param pid the process-id with which the file should be opened
	 * @param flags whether it is a virtual or real file and whether you want to read or write
	 * @param mode the mode to set (if a file is created by this call)
	 * @param path the path
	 * @param file will be set to the opened file
	 * @return 0 if successfull or < 0
	 */
	static int openPath(pid_t pid,ushort flags,mode_t mode,const char *path,OpenFile **file);

	/**
	 * Opens the file with given number and given flags. That means it walks through the global
	 * file table and searches for a free entry or an entry for that file.
	 * Note that multiple processs may read from the same file simultaneously but NOT write!
	 *
	 * @param pid the process-id with which the file should be opened
	 * @param flags whether it is a virtual or real file and whether you want to read or write
	 * @param node the node if its virtual or NULL otherwise.
	 * @param nodeNo the node-number (in the virtual or real environment)
	 * @param devNo the device-number
	 * @param file will be set to the opened file
	 * @return 0 if successfull or < 0
	 */
	static int openFile(pid_t pid,ushort flags,const VFSNode *node,ino_t nodeNo,dev_t devNo,
	                    OpenFile **file);

	/**
	 * Retrieves information about the given path
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param info the info to fill
	 * @return 0 on success
	 */
	static int stat(pid_t pid,const char *path,struct stat *info);

	/**
	 * Sets the permissions of the file denoted by <path> to <mode>.
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param mode the new mode
	 * @return 0 on success
	 */
	static int chmod(pid_t pid,const char *path,mode_t mode);

	/**
	 * Sets the owner and group of the file denoted by <path>
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param uid the new user-id
	 * @param gid the new group-id
	 * @return 0 on success
	 */
	static int chown(pid_t pid,const char *path,uid_t uid,gid_t gid);

	/**
	 * Sets the access and modification times of the file denoted by <path>
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param utimes the new access and modification times
	 * @return 0 on success
	 */
	static int utime(pid_t pid,const char *path,const struct utimbuf *utimes);

	/**
	 * Creates a link @ <newPath> to <oldPath>
	 *
	 * @param pid the process-id
	 * @param oldPath the link-target
	 * @param newPath the link-name
	 * @return 0 on success
	 */
	static int link(pid_t pid,const char *oldPath,const char *newPath);

	/**
	 * Removes the given file
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @return 0 on success
	 */
	static int unlink(pid_t pid,const char *path);

	/**
	 * Renames <oldPath> to <newPath>.
	 *
	 * @param pid the process-id
	 * @param oldPath the link-target
	 * @param newPath the link-name
	 * @return 0 on success
	 */
	static int rename(pid_t pid,const char *oldPath,const char *newPath);

	/**
	 * Creates the directory <path>
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param mode the mode to set
	 * @return 0 on success
	 */
	static int mkdir(pid_t pid,const char *path,mode_t mode);

	/**
	 * Removes the given directory
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @return 0 on success
	 */
	static int rmdir(pid_t pid,const char *path);

	/**
	 * Creates a device-node for the given process at given path and opens a file for it
	 *
	 * @param pid the process-id
	 * @param path the path to the device
	 * @param mode the mode to set
	 * @param type the device-type (DEV_TYPE_*)
	 * @param ops the supported operations
	 * @param file will be set to the opened file
	 * @return 0 if ok, negative if an error occurred
	 */
	static int createdev(pid_t pid,char *path,mode_t mode,uint type,uint ops,OpenFile **file);

	/**
	 * Creates a new sibling-channel for <file>.
	 *
	 * @param pid the process-id
	 * @param file the current channel
	 * @param arg an arbitrary argument to send to the driver
	 * @param sibl will be set to the created sibling channel
	 * @return 0 on success
	 */
	static int creatsibl(pid_t pid,OpenFile *file,int arg,OpenFile **sibl);

	/**
	 * Creates a process-node with given pid
	 *
	 * @param pid the process-id
	 * @param ms the mountspace of the process
	 * @return 0 on success
	 */
	static ino_t createProcess(pid_t pid,VFSNode *ms);

	/**
	 * Removes all occurrences of the given process from VFS
	 *
	 * @param pid the process-id
	 */
	static void removeProcess(pid_t pid);

	/**
	 * Creates the VFS-node for the given thread
	 *
	 * @param tid the thread-id
	 * @return the inode-number for the directory-node on success
	 */
	static ino_t createThread(tid_t tid);

	/**
	 * Removes all occurrences of the given thread from VFS
	 *
	 * @param tid the thread-id
	 */
	static void removeThread(tid_t tid);

	/**
	 * Prints all messages of all devices
	 *
	 * @param os the output-stream
	 */
	static void printMsgs(OStream &os);

private:
	static bool hasMsg(VFSNode *node);
	static bool hasData(VFSNode *node);
	static bool hasWork(VFSNode *node);
	static int request(pid_t pid,const char *path,ushort flags,mode_t mode,const char **begin,
		OpenFile **res);

	static VFSNode *procsNode;
	static VFSNode *devNode;
	static VFSNode *msNode;
};
