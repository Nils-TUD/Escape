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
#include <sys/vfs/node.h>
#include <esc/fsinterface.h>

#define MAX_VFS_FILE_SIZE			(128 * K)
#define MAX_GETWORK_DEVICES			16

#define DEV_OPEN					1
#define DEV_READ					2
#define DEV_WRITE					4
#define DEV_CLOSE					8
#define DEV_SHFILE					16

#define DEV_TYPE_FS					0
#define DEV_TYPE_BLOCK				1
#define DEV_TYPE_CHAR				2
#define DEV_TYPE_FILE				3
#define DEV_TYPE_SERVICE			4

/* fcntl-commands */
#define F_GETFL						0
#define F_SETFL						1
#define F_SETDATA					2
#define F_GETACCESS					3
#define F_SETUNUSED					4
#define F_SEMUP						5
#define F_SEMDOWN					6

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* getwork-flags */
#define GW_NOBLOCK					1
#define GW_MARKUSED					2

/* all flags that the user can use */
#define VFS_USER_FLAGS				(VFS_WRITE | VFS_READ | VFS_MSGS | VFS_CREATE | VFS_TRUNCATE | \
									 VFS_APPEND | VFS_NOBLOCK | VFS_EXCLUSIVE)

class Proc;

class VFS {
	VFS() = delete;

public:
	/**
	 * Initializes the virtual file system
	 */
	static void init();

	/**
	 * Mounts the virtual filesystems into the given process. Should only be used by init.
	 *
	 * @param p the process
	 */
	static void mountAll(Proc *p);

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
	 * Creates a pipe and opens one file for reading and one file for writing.
	 *
	 * @param pid the process-id
	 * @param readFile will be set to the file for reading
	 * @param writeFile will be set to the file for writing
	 * @return 0 on success
	 */
	static int openPipe(pid_t pid,OpenFile **readFile,OpenFile **writeFile);

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
	static int openFile(pid_t pid,ushort flags,const VFSNode *node,inode_t nodeNo,dev_t devNo,
	                    OpenFile **file);

	/**
	 * Retrieves information about the given path
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @param info the info to fill
	 * @return 0 on success
	 */
	static int stat(pid_t pid,const char *path,sFileInfo *info);

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
	 * Creates the directory <path>
	 *
	 * @param pid the process-id
	 * @param path the path
	 * @return 0 on success
	 */
	static int mkdir(pid_t pid,const char *path);

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
	 * Waits for the given event. First, the function checks whether we can wait, i.e. if the event
	 * to wait for has already arrived. If not, we wait until one of the events arrived.
	 * If <pid> != KERNEL_PID, it calls Lock::release(pid,ident) before going to sleep (this is used
	 * for waitunlock).
	 *
	 * @param event the event to wait for
	 * @param object the object to wait for (is expected to be a OpenFile*, if its a file-wait)
	 * @param maxWaitTime the maximum time to wait (in milliseconds)
	 * @param block whether we should wait if necessary (otherwise it will be checked only whether
	 *  we can wait and if so, -EWOULDBLOCK is returned. if not, 0 is returned.)
	 * @param pid the process-id for Lock::release (KERNEL_PID = don't call it)
	 * @param ident the ident for lock_release
	 * @return 0 on success
	 */
	static int waitFor(uint event,evobj_t object,time_t maxWaitTime,bool block,pid_t pid,ulong ident);

	/**
	 * Creates a process-node with given pid
	 *
	 * @param pid the process-id
	 * @return 0 on success
	 */
	static inode_t createProcess(pid_t pid);

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
	static inode_t createThread(tid_t tid);

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
};
