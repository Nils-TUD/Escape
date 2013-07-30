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
#include <sys/task/event.h>
#include <esc/sllist.h>
#include <esc/fsinterface.h>

#define MAX_VFS_FILE_SIZE			(64 * K)
#define MAX_GETWORK_DEVICES			16

/* some additional types for the kernel */
#define MODE_TYPE_CHANNEL			0x0010000
#define MODE_TYPE_PIPE				0x0020000
#define MODE_TYPE_DEVMASK			0x0700000
#define MODE_TYPE_BLKDEV			(0x0100000 | S_IFBLK)
#define MODE_TYPE_CHARDEV			(0x0200000 | S_IFCHR)
#define MODE_TYPE_FSDEV				(0x0300000 | S_IFFS)
#define MODE_TYPE_FILEDEV			(0x0400000 | S_IFREG)
#define MODE_TYPE_SERVDEV			(0x0500000 | S_IFSERV)

/* the device-number of the VFS */
#define VFS_DEV_NO					((dev_t)0xFF)

#define IS_DEVICE(mode)				(((mode) & MODE_TYPE_DEVMASK) != 0)
#define IS_CHANNEL(mode)			(((mode) & MODE_TYPE_CHANNEL) != 0)
#define IS_FS(mode)					(((mode) & MODE_TYPE_DEVMASK) == 0x0300000)

#define DEV_OPEN					1
#define DEV_READ					2
#define DEV_WRITE					4
#define DEV_CLOSE					8

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

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* getwork-flags */
#define GW_NOBLOCK					1

/* GFT flags */
enum {
	VFS_NOACCESS = 0,		/* no read and write */
	VFS_READ = 1,
	VFS_WRITE = 2,
	VFS_CREATE = 4,
	VFS_TRUNCATE = 8,
	VFS_APPEND = 16,
	VFS_NOBLOCK = 32,
	VFS_MSGS = 64,			/* exchange msgs with a device */
	VFS_EXCLUSIVE = 128,	/* disallow other accesses */
	VFS_NOLINKRES = 256,	/* kernel-intern: don't resolve last link in path */
	VFS_DEVICE = 512,		/* kernel-intern: whether the file was created for a device */
	VFS_EXEC = 1024,		/* kernel-intern: for accessing directories */
};

/* all flags that the user can use */
#define VFS_USER_FLAGS				(VFS_WRITE | VFS_READ | VFS_MSGS | VFS_CREATE | VFS_TRUNCATE | \
									 VFS_APPEND | VFS_NOBLOCK | VFS_EXCLUSIVE)

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;

/* the prototypes for the operations on nodes */
typedef ssize_t (*fRead)(pid_t pid,OpenFile *file,sVFSNode *node,void *buffer,
			off_t offset,size_t count);
typedef ssize_t (*fWrite)(pid_t pid,OpenFile *file,sVFSNode *node,const void *buffer,
			off_t offset,size_t count);
typedef off_t (*fSeek)(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
typedef size_t (*fGetSize)(pid_t pid,sVFSNode *node);
typedef void (*fClose)(pid_t pid,OpenFile *file,sVFSNode *node);
typedef void (*fDestroy)(sVFSNode *n);

struct sVFSNode {
	klock_t lock;
	const char *const name;
	const size_t nameLen;
	/* number of open files for this node */
	ushort refCount;
	/* the owner of this node */
	const pid_t owner;
	uid_t uid;
	gid_t gid;
	/* 0 means unused; stores permissions and the type of node */
	uint mode;
	/* for the vfs-structure */
	sVFSNode *const parent;
	sVFSNode *prev;
	sVFSNode *next;
	sVFSNode *firstChild;
	sVFSNode *lastChild;
	/* the handler that perform a read-/write-operation on this node */
	fRead read;
	fWrite write;
	fSeek seek;
	fGetSize getSize;
	fClose close;
	fDestroy destroy;
	/* data in various forms, depending on the node-type */
	void *data;
};

/**
 * Initializes the virtual file system
 */
void vfs_init(void);

/**
 * Checks whether the process with given id has permission to use <n> with the given flags
 *
 * @param pid the process-id
 * @param n the node
 * @param flags the flags (VFS_*)
 * @return 0 if successfull, negative if the process has no permission
 */
int vfs_hasAccess(pid_t pid,sVFSNode *n,ushort flags);

/**
 * Opens the given path with given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processs may read from the same file simultaneously but NOT write!
 *
 * @param pid the process-id with which the file should be opened
 * @param flags whether it is a virtual or real file and whether you want to read or write
 * @param path the path
 * @param file will be set to the opened file
 * @return 0 if successfull or < 0
 */
int vfs_openPath(pid_t pid,ushort flags,const char *path,OpenFile **file);

/**
 * Creates a pipe and opens one file for reading and one file for writing.
 *
 * @param pid the process-id
 * @param readFile will be set to the file for reading
 * @param writeFile will be set to the file for writing
 * @return 0 on success
 */
int vfs_openPipe(pid_t pid,OpenFile **readFile,OpenFile **writeFile);

/**
 * Opens the file with given number and given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processs may read from the same file simultaneously but NOT write!
 *
 * @param pid the process-id with which the file should be opened
 * @param flags whether it is a virtual or real file and whether you want to read or write
 * @param nodeNo the node-number (in the virtual or real environment)
 * @param devNo the device-number
 * @param file will be set to the opened file
 * @return 0 if successfull or < 0
 */
int vfs_openFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,OpenFile **file);

/**
 * Retrieves information about the given path
 *
 * @param pid the process-id
 * @param path the path
 * @param info the info to fill
 * @return 0 on success
 */
int vfs_stat(pid_t pid,const char *path,sFileInfo *info);

/**
 * Sets the permissions of the file denoted by <path> to <mode>.
 *
 * @param pid the process-id
 * @param path the path
 * @param mode the new mode
 * @return 0 on success
 */
int vfs_chmod(pid_t pid,const char *path,mode_t mode);

/**
 * Sets the owner and group of the file denoted by <path>
 *
 * @param pid the process-id
 * @param path the path
 * @param uid the new user-id
 * @param gid the new group-id
 * @return 0 on success
 */
int vfs_chown(pid_t pid,const char *path,uid_t uid,gid_t gid);

/**
 * Mounts <device> at <path> with given type.
 *
 * @param pid the process-id
 * @param device the device to mount
 * @param path the path to mount the device at
 * @param type the type of file-system to use
 * @return 0 on success
 */
int vfs_mount(pid_t pid,const char *device,const char *path,uint type);

/**
 * Unmounts the given path
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_unmount(pid_t pid,const char *path);

/**
 * Writes all cached blocks to disk.
 *
 * @param pid the process-id
 */
int vfs_sync(pid_t pid);

/**
 * Creates a link @ <newPath> to <oldPath>
 *
 * @param pid the process-id
 * @param oldPath the link-target
 * @param newPath the link-name
 * @return 0 on success
 */
int vfs_link(pid_t pid,const char *oldPath,const char *newPath);

/**
 * Removes the given file
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_unlink(pid_t pid,const char *path);

/**
 * Creates the directory <path>
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_mkdir(pid_t pid,const char *path);

/**
 * Removes the given directory
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_rmdir(pid_t pid,const char *path);

/**
 * Creates a device-node for the given process at given path and opens a file for it
 *
 * @param pid the process-id
 * @param path the path to the device
 * @param type the device-type (DEV_TYPE_*)
 * @param ops the supported operations
 * @param file will be set to the opened file
 * @return 0 if ok, negative if an error occurred
 */
int vfs_createdev(pid_t pid,char *path,uint type,uint ops,OpenFile **file);

/**
 * Waits for the given wait-objects, whereas the objects are expected to be of type OpenFile*.
 * First, the function checks whether we can wait, i.e. if the event to wait for has already
 * arrived. If not, we wait until one of the events arrived.
 * If <pid> != KERNEL_PID, it calls Lock::release(pid,ident) before going to sleep (this is used
 * for waitunlock).
 *
 * @param objects the array of wait-objects (will be changed; files -> nodes)
 * @param objCount the number of wait-objects
 * @param maxWaitTime the maximum time to wait (in milliseconds)
 * @param block whether we should wait if necessary (otherwise it will be checked only whether
 *  we can wait and if so, -EWOULDBLOCK is returned. if not, 0 is returned.)
 * @param pid the process-id for Lock::release (KERNEL_PID = don't call it)
 * @param ident the ident for lock_release
 * @return 0 on success
 */
int vfs_waitFor(Event::WaitObject *objects,size_t objCount,time_t maxWaitTime,bool block,
		pid_t pid,ulong ident);

/**
 * Creates a process-node with given pid
 *
 * @param pid the process-id
 * @return the process-directory-node on success
 */
inode_t vfs_createProcess(pid_t pid);

/**
 * Removes all occurrences of the given process from VFS
 *
 * @param pid the process-id
 */
void vfs_removeProcess(pid_t pid);

/**
 * Creates the VFS-node for the given thread
 *
 * @param tid the thread-id
 * @return true on success
 */
bool vfs_createThread(tid_t tid);

/**
 * Removes all occurrences of the given thread from VFS
 *
 * @param tid the thread-id
 */
void vfs_removeThread(tid_t tid);

/**
 * Prints all messages of all devices
 *
 * @param os the output-stream
 */
void vfs_printMsgs(OStream &os);
