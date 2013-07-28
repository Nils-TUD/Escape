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
typedef ssize_t (*fRead)(pid_t pid,sFile *file,sVFSNode *node,void *buffer,
			off_t offset,size_t count);
typedef ssize_t (*fWrite)(pid_t pid,sFile *file,sVFSNode *node,const void *buffer,
			off_t offset,size_t count);
typedef off_t (*fSeek)(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
typedef size_t (*fGetSize)(pid_t pid,sVFSNode *node);
typedef void (*fClose)(pid_t pid,sFile *file,sVFSNode *node);
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
 * Checks whether they point to the same file
 *
 * @param f1 the first file
 * @param f2 the second file
 * @return true if device and node number are the same
 */
bool vfs_isSameFile(sFile *f1,sFile *f2);

/**
 * @param f the file
 * @return the path with which the given file has been opened
 */
const char *vfs_getPath(sFile *f);

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
 * @param file the file
 * @return the node of the given file (might be NULL)
 */
sVFSNode *vfs_getNode(sFile *file);

/**
 * @param file the file
 * @return true if the file was created for a device (and not a client of the device)
 */
bool vfs_isDevice(sFile *file);

/**
 * Increases the references of the given file
 *
 * @param file the file
 */
void vfs_incRefs(sFile *file);

/**
 * Increments the number of usages of the given file
 *
 * @param file the file
 */
void vfs_incUsages(sFile *file);

/**
 * Decrements the number of usages of the given file
 *
 * @param file the file
 */
void vfs_decUsages(sFile *file);

/**
 * Manipulates the given file, depending on the command
 *
 * @param pid the process-id
 * @param file the file
 * @param cmd the command (F_GETFL or F_SETFL)
 * @param arg the argument (just used for F_SETFL)
 * @return >= 0 on success
 */
int vfs_fcntl(pid_t pid,sFile *file,uint cmd,int arg);

/**
 * @param file the file
 * @return whether the file should be used in blocking-mode
 */
bool vfs_shouldBlock(sFile *file);

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
int vfs_openPath(pid_t pid,ushort flags,const char *path,sFile **file);

/**
 * Creates a pipe and opens one file for reading and one file for writing.
 *
 * @param pid the process-id
 * @param readFile will be set to the file for reading
 * @param writeFile will be set to the file for writing
 * @return 0 on success
 */
int vfs_openPipe(pid_t pid,sFile **readFile,sFile **writeFile);

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
int vfs_openFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,sFile **file);

/**
 * Returns the current file-position
 *
 * @param pid the process-id
 * @param file the file
 * @return the current file-position
 */
off_t vfs_tell(pid_t pid,sFile *file);

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
 * Retrieves information about the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param info the info to fill
 * @return 0 on success
 */
int vfs_fstat(pid_t pid,sFile *file,sFileInfo *info);

/**
 * Sets the position for the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param offset the offset
 * @param whence the seek-type
 * @return the new position on success
 */
off_t vfs_seek(pid_t pid,sFile *file,off_t offset,uint whence);

/**
 * Reads max. count bytes from the given file into the given buffer and returns the number
 * of read bytes.
 *
 * @param pid will be used to check whether the device writes or a device-user
 * @param file the file
 * @param buffer the buffer to write to
 * @param count the max. number of bytes to read
 * @return the number of bytes read
 */
ssize_t vfs_readFile(pid_t pid,sFile *file,void *buffer,size_t count);

/**
 * Writes count bytes from the given buffer into the given file and returns the number of written
 * bytes.
 *
 * @param pid will be used to check whether the device writes or a device-user
 * @param file the file
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written
 */
ssize_t vfs_writeFile(pid_t pid,sFile *file,const void *buffer,size_t count);

/**
 * Sends a message to the corresponding device
 *
 * @param pid the sender-process-id
 * @param file the file to send the message to
 * @param id the message-id
 * @param data1 the message-data
 * @param size1 the data-size
 * @param data2 for the device-messages: a second message (NULL = no second one)
 * @param size2 the size of the second message
 * @return 0 on success
 */
ssize_t vfs_sendMsg(pid_t pid,sFile *file,msgid_t id,USER const void *data1,size_t size1,
		USER const void *data2,size_t size2);

/**
 * Receives a message from the corresponding device
 *
 * @param pid the receiver-process-id
 * @param file the file to receive the message from
 * @param id will be set to the fetched msg-id
 * @param data the message to write to
 * @param forceBlock if set, the function will block regardless of the NOBLOCK-flag in file
 * @return the number of written bytes (or < 0 if an error occurred)
 */
ssize_t vfs_receiveMsg(pid_t pid,sFile *file,msgid_t *id,void *data,size_t size,bool forceBlock);

/**
 * Closes the given file. That means it calls Proc::closeFile() and decrements the reference-count
 * in the global file table. If there are no references anymore it releases the slot.
 *
 * @param pid the process-id
 * @param file the file
 * @return true if the file has really been closed
 */
bool vfs_closeFile(pid_t pid,sFile *file);

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
int vfs_createdev(pid_t pid,char *path,uint type,uint ops,sFile **file);

/**
 * Waits for the given wait-objects, whereas the objects are expected to be of type sFile*.
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
 * For devices: Looks whether a client wants to be served and return the node-number. If
 * necessary and if GW_NOBLOCK is not used, the function waits until a client wants to be served.
 *
 * @param files an array of files to check for clients
 * @param count the number of files
 * @param index will be set to the index in <files> from which the client was chosen
 * @param flags the flags (GW_*)
 * @return the error-code or the node-number of the client
 */
inode_t vfs_getClient(sFile *const *files,size_t count,size_t *index,uint flags);

/***
 * Fetches the client-id from the given file
 *
 * @param pid the process-id (device-owner)
 * @param file the file for the client
 * @return the client-id or the error-code
 */
inode_t vfs_getClientId(pid_t pid,sFile *file);

/**
 * Opens a file for the client with given process-id.
 *
 * @param pid the own process-id
 * @param file the file for the device
 * @param clientId the id of the desired client
 * @param cfile will be set to the opened file
 * @return 0 or a negative error-code
 */
int vfs_openClient(pid_t pid,sFile *file,inode_t clientId,sFile **cfile);

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
 * Prints all used entries in the global file table
 *
 * @param os the output-stream
 */
void vfs_printGFT(OStream &os);

/**
 * Prints all messages of all devices
 *
 * @param os the output-stream
 */
void vfs_printMsgs(OStream &os);

/**
 * Prints information to the given file
 *
 * @param os the output-stream
 * @param f the file
 */
void vfs_printFile(OStream &os,sFile *f);

/**
 * @return the number of entries in the global file table
 */
size_t vfs_dbg_getGFTEntryCount(void);
