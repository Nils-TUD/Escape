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

#include <sys/driver.h>
#include <sys/io.h>
#include <vfs/node.h>
#include <common.h>

class Proc;
class VFSMS;

class VFS {
	VFS() = delete;

public:
	static const size_t MAX_FILE_SIZE 	= 1024 * 1024;

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
	 * Checks whether the process with given id has permission with the given flags
	 *
	 * @param pid the process-id
	 * @param mode the mode of the object to get permission to
	 * @param uid the uid of the object to get permission to
	 * @param gid the gid of the object to get permission to
	 * @param flags the flags (VFS_*)
	 * @return 0 if successfull, negative if the process has no permission
	 */
	static int hasAccess(pid_t pid,mode_t mode,uid_t uid,gid_t gid,ushort flags);

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
	 * Opens the given file for process <pid> and associates a file descriptor for the process
	 * with that file.
	 *
	 * @param pid the process-id with which the file should be opened
	 * @param flags whether it is a virtual or real file and whether you want to read or write
	 * @param node the node if its virtual or NULL otherwise.
	 * @param nodeNo the node-number (in the virtual or real environment)
	 * @param devNo the device-number
	 * @return the file descriptor on success
	 */
	static int openFileDesc(pid_t pid,ushort flags,const VFSNode *node,ino_t nodeNo,dev_t devNo);

	/**
	 * Closes the given file descriptor for process <pid>.
	 *
	 * @param pid the process id
	 * @param fd the file descriptor
	 */
	static void closeFileDesc(pid_t pid,int fd);

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
	static VFSNode *tmpNode;
	static VFSNode *msNode;
};
