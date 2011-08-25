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

#ifndef VFS_H_
#define VFS_H_

#include <sys/common.h>
#include <sys/task/event.h>
#include <esc/sllist.h>
#include <esc/fsinterface.h>

#define MAX_VFS_FILE_SIZE			(64 * K)
#define MAX_GETWORK_DRIVERS			16

/* some additional types for the kernel */
#define MODE_TYPE_CHANNEL			0x0010000
#define MODE_TYPE_DRIVER			0x0020000
#define MODE_TYPE_PIPECON			0x0040000
#define MODE_TYPE_PIPE				0x0080000

/* the device-number of the VFS */
#define VFS_DEV_NO					((dev_t)0xFF)

#define IS_DRIVER(mode)				(((mode) & MODE_TYPE_DRIVER) != 0)
#define IS_CHANNEL(mode)			(((mode) & MODE_TYPE_CHANNEL) != 0)

#define DRV_OPEN					1
#define DRV_READ					2
#define DRV_WRITE					4
#define DRV_CLOSE					8
#define DRV_FS						16

/* fcntl-commands */
#define F_GETFL						0
#define F_SETFL						1
#define F_SETDATA					2
#define F_GETACCESS					3

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* getWork-flags */
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
	VFS_MSGS = 64,			/* exchange msgs with a driver */
	VFS_NOLINKRES = 128,	/* kernel-intern: don't resolve last link in path */
	VFS_DRIVER = 256,		/* kernel-intern: whether the file was created for a driver */
	VFS_EXEC = 512,			/* kernel-intern: for accessing directories */
};

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;

/* the prototypes for the operations on nodes */
typedef ssize_t (*fRead)(pid_t pid,file_t file,sVFSNode *node,void *buffer,
			off_t offset,size_t count);
typedef ssize_t (*fWrite)(pid_t pid,file_t file,sVFSNode *node,const void *buffer,
			off_t offset,size_t count);
typedef off_t (*fSeek)(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
typedef void (*fClose)(pid_t pid,file_t file,sVFSNode *node);
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
 * @param file the file
 * @return true if the file was created for a driver (and not a client of the driver)
 */
bool vfs_isDriver(file_t file);

/**
 * Increases the references of the given file
 *
 * @param file the file
 */
void vfs_incRefs(file_t file);

/**
 * Increments the number of usages of the given file
 *
 * @param file the file
 */
void vfs_incUsages(file_t file);

/**
 * Decrements the number of usages of the given file
 *
 * @param file the file
 */
void vfs_decUsages(file_t file);

/**
 * Manipulates the given file, depending on the command
 *
 * @param pid the process-id
 * @param file the file
 * @param cmd the command (F_GETFL or F_SETFL)
 * @param arg the argument (just used for F_SETFL)
 * @return >= 0 on success
 */
int vfs_fcntl(pid_t pid,file_t file,uint cmd,int arg);

/**
 * @param file the file
 * @return whether the file should be used in blocking-mode
 */
bool vfs_shouldBlock(file_t file);

/**
 * Opens the given path with given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processs may read from the same file simultaneously but NOT write!
 *
 * @param pid the process-id with which the file should be opened
 * @param flags whether it is a virtual or real file and whether you want to read or write
 * @param path the path
 * @return the file if successfull or < 0 (ERR_FILE_IN_USE, ERR_NO_FREE_FILE)
 */
file_t vfs_openPath(pid_t pid,ushort flags,const char *path);

/**
 * Creates a pipe and opens one file for reading and one file for writing.
 *
 * @param pid the process-id
 * @param readFile will be set to the file for reading
 * @param writeFile will be set to the file for writing
 * @return 0 on success
 */
int vfs_openPipe(pid_t pid,file_t *readFile,file_t *writeFile);

/**
 * Opens the file with given number and given flags. That means it walks through the global
 * file table and searches for a free entry or an entry for that file.
 * Note that multiple processs may read from the same file simultaneously but NOT write!
 *
 * @param pid the process-id with which the file should be opened
 * @param flags whether it is a virtual or real file and whether you want to read or write
 * @param nodeNo the node-number (in the virtual or real environment)
 * @param devNo the device-number
 * @return the file if successfull or < 0 (ERR_FILE_IN_USE, ERR_NO_FREE_FILE)
 */
file_t vfs_openFile(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo);

/**
 * Returns the current file-position
 *
 * @param pid the process-id
 * @param file the file
 * @return the current file-position
 */
off_t vfs_tell(pid_t pid,file_t file);

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
int vfs_fstat(pid_t pid,file_t file,sFileInfo *info);

/**
 * Sets the position for the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param offset the offset
 * @param whence the seek-type
 * @return the new position on success
 */
off_t vfs_seek(pid_t pid,file_t file,off_t offset,uint whence);

/**
 * Reads max. count bytes from the given file into the given buffer and returns the number
 * of read bytes.
 *
 * @param pid will be used to check whether the driver writes or a driver-user
 * @param file the file
 * @param buffer the buffer to write to
 * @param count the max. number of bytes to read
 * @return the number of bytes read
 */
ssize_t vfs_readFile(pid_t pid,file_t file,void *buffer,size_t count);

/**
 * Writes count bytes from the given buffer into the given file and returns the number of written
 * bytes.
 *
 * @param pid will be used to check whether the driver writes or a driver-user
 * @param file the file
 * @param buffer the buffer to read from
 * @param count the number of bytes to write
 * @return the number of bytes written
 */
ssize_t vfs_writeFile(pid_t pid,file_t file,const void *buffer,size_t count);

/**
 * Sends a message to the corresponding driver
 *
 * @param pid the sender-process-id
 * @param file the file to send the message to
 * @param id the message-id
 * @param data the message
 * @param size the message-size
 * @return 0 on success
 */
ssize_t vfs_sendMsg(pid_t pid,file_t file,msgid_t id,const void *data,size_t size);

/**
 * Receives a message from the corresponding driver
 *
 * @param pid the receiver-process-id
 * @param file the file to receive the message from
 * @param id will be set to the fetched msg-id
 * @param data the message to write to
 * @return the number of written bytes (or < 0 if an error occurred)
 */
ssize_t vfs_receiveMsg(pid_t pid,file_t file,msgid_t *id,void *data,size_t size);

/**
 * Closes the given file. That means it calls proc_closeFile() and decrements the reference-count
 * in the global file table. If there are no references anymore it releases the slot.
 *
 * @param pid the process-id
 * @param file the file
 * @return true if the file has really been closed
 */
bool vfs_closeFile(pid_t pid,file_t file);

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
 * Creates a driver-node for the given process and given name and opens a file for it
 *
 * @param pid the process-id
 * @param name the driver-name
 * @param flags the specified flags (implemented functions)
 * @return the file-number if ok, negative if an error occurred
 */
file_t vfs_createDriver(pid_t pid,const char *name,uint flags);

/**
 * Waits for the given wait-objects, whereas the objects are expected to be of type file_t.
 * First, the function checks whether we can wait, i.e. if the event to wait for has already
 * arrived. If not, we wait until one of the events arrived.
 * If <pid> != KERNEL_PID, it calls lock_release(pid,ident) before going to sleep (this is used
 * for waitUnlock).
 *
 * @param objects the array of wait-objects (will be changed; files -> nodes)
 * @param objCount the number of wait-objects
 * @param block whether we should wait if necessary (otherwise it will be checked only whether
 *  we can wait and if so, ERR_WOULD_BLOCKED is returned. if not, 0 is returned.)
 * @param pid the process-id for lock_release (KERNEL_PID = don't call it)
 * @param ident the ident for lock_release
 * @return 0 on success
 */
int vfs_waitFor(sWaitObject *objects,size_t objCount,bool block,pid_t pid,ulong ident);

/**
 * For drivers: Looks whether a client wants to be served and return the node-number. If
 * necessary and if GW_NOBLOCK is not used, the function waits until a client wants to be served.
 *
 * @param files an array of files to check for clients
 * @param count the number of files
 * @param index will be set to the index in <files> from which the client was chosen
 * @param flags the flags (GW_*)
 * @return the error-code or the node-number of the client
 */
inode_t vfs_getClient(const file_t *files,size_t count,size_t *index,uint flags);

/***
 * Fetches the client-id from the given file
 *
 * @param pid the process-id (driver-owner)
 * @param file the file for the client
 * @return the client-id or the error-code
 */
inode_t vfs_getClientId(pid_t pid,file_t file);

/**
 * Opens a file for the client with given process-id.
 *
 * @param pid the own process-id
 * @param file the file for the driver
 * @param clientId the id of the desired client
 * @return the file or a negative error-code
 */
file_t vfs_openClient(pid_t pid,file_t file,inode_t clientId);

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
 */
void vfs_printGFT(void);

/**
 * Prints all messages of all drivers
 */
void vfs_printMsgs(void);

/**
 * Prints information to the given file
 *
 * @param file the file
 */
void vfs_printFile(file_t file);

/**
 * @return the number of entries in the global file table
 */
size_t vfs_dbg_getGFTEntryCount(void);

#endif /* VFS_H_ */
