/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/sllist.h>
#include <esc/fsinterface.h>

#define MAX_VFS_FILE_SIZE			(64 * K)

/* some additional types for the kernel */
#define MODE_TYPE_CHANNEL			0x0010000
#define MODE_TYPE_DRIVER			0x0020000
#define MODE_TYPE_PIPECON			0x0040000
#define MODE_TYPE_PIPE				0x0080000

/* the device-number of the VFS */
#define VFS_DEV_NO					((tDevNo)0xFF)

#define IS_DRIVER(mode)				(((mode) & MODE_TYPE_DRIVER) != 0)
#define IS_CHANNEL(mode)			(((mode) & MODE_TYPE_CHANNEL) != 0)

#define DRV_OPEN					1
#define DRV_READ					2
#define DRV_WRITE					4
#define DRV_CLOSE					8
#define DRV_FS						16
#define DRV_TERM					32

/* fcntl-commands */
#define F_GETFL						0
#define F_SETFL						1
#define F_SETDATA					2
#define F_GETACCESS					3

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* GFT flags */
enum {
	VFS_NOACCESS = 0,		/* no read and write */
	VFS_READ = 1,
	VFS_WRITE = 2,
	VFS_CREATE = 4,
	VFS_TRUNCATE = 8,
	VFS_APPEND = 16,
	VFS_NOBLOCK = 32,
	VFS_NOLINKRES = 64,		/* kernel-intern: don't resolve last link in path */
	VFS_DRIVER = 128,		/* kernel-intern: wether the file was created for a driver */
};

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;

/* the prototypes for the operations on nodes */
typedef ssize_t (*fRead)(tPid pid,tFileNo file,sVFSNode *node,void *buffer,
			off_t offset,size_t count);
typedef ssize_t (*fWrite)(tPid pid,tFileNo file,sVFSNode *node,const void *buffer,
			off_t offset,size_t count);
typedef off_t (*fSeek)(tPid pid,sVFSNode *node,off_t position,off_t offset,uint whence);
typedef void (*fClose)(tPid pid,tFileNo file,sVFSNode *node);
typedef void (*fDestroy)(sVFSNode *n);

struct sVFSNode {
	char *name;
	/* number of open files for this node */
	ushort refCount;
	/* the owner of this node */
	tPid owner;
	/* 0 means unused; stores permissions and the type of node */
	uint mode;
	/* for the vfs-structure */
	sVFSNode *parent;
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
 * @param file the file
 * @return true if the file was created for a driver (and not a client of the driver)
 */
bool vfs_isDriver(tFileNo file);

/**
 * Increases the references of the given file
 *
 * @param file the file
 * @return 0 on success
 */
int vfs_incRefs(tFileNo file);

/**
 * @param file the file-number
 * @return the virtual node associated with the given file or NULL if failed
 */
sVFSNode *vfs_getVNode(tFileNo file);

/**
 * Determines the node-number and device-number for the given file
 *
 * @param file the file
 * @param ino will be set to the inode-number
 * @param dev will be set to the devive-number
 * @return 0 on success
 */
int vfs_getFileId(tFileNo file,tInodeNo *ino,tDevNo *dev);

/**
 * Manipulates the given file, depending on the command
 *
 * @param pid the process-id
 * @param file the file
 * @param cmd the command (F_GETFL or F_SETFL)
 * @param arg the argument (just used for F_SETFL)
 * @return >= 0 on success
 */
int vfs_fcntl(tPid pid,tFileNo file,uint cmd,int arg);

/**
 * @param file the file
 * @return wether the file should be used in blocking-mode
 */
bool vfs_shouldBlock(tFileNo file);

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
tFileNo vfs_openPath(tPid pid,ushort flags,const char *path);

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
tFileNo vfs_openFile(tPid pid,ushort flags,tInodeNo nodeNo,tDevNo devNo);

/**
 * Returns the current file-position
 *
 * @param pid the process-id
 * @param file the file
 * @return the current file-position
 */
off_t vfs_tell(tPid pid,tFileNo file);

/**
 * Retrieves information about the given path
 *
 * @param pid the process-id
 * @param path the path
 * @param info the info to fill
 * @return 0 on success
 */
int vfs_stat(tPid pid,const char *path,sFileInfo *info);

/**
 * Retrieves information about the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param info the info to fill
 * @return 0 on success
 */
int vfs_fstat(tPid pid,tFileNo file,sFileInfo *info);

/**
 * Checks whether the given file links to a terminal. That means it has to be a virtual file
 * that acts as a driver-client for a terminal-driver.
 *
 * @param pid the process-id
 * @param file the file
 * @return true if so
 */
bool vfs_isterm(tPid pid,tFileNo file);

/**
 * Sets the position for the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param offset the offset
 * @param whence the seek-type
 * @return the new position on success
 */
off_t vfs_seek(tPid pid,tFileNo file,off_t offset,uint whence);

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
ssize_t vfs_readFile(tPid pid,tFileNo file,void *buffer,size_t count);

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
ssize_t vfs_writeFile(tPid pid,tFileNo file,const void *buffer,size_t count);

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
ssize_t vfs_sendMsg(tPid pid,tFileNo file,tMsgId id,const void *data,size_t size);

/**
 * Receives a message from the corresponding driver
 *
 * @param pid the receiver-process-id
 * @param file the file to receive the message from
 * @param id will be set to the fetched msg-id
 * @param data the message to write to
 * @return the number of written bytes (or < 0 if an error occurred)
 */
ssize_t vfs_receiveMsg(tPid pid,tFileNo file,tMsgId *id,void *data,size_t size);

/**
 * Closes the given file. That means it calls proc_closeFile() and decrements the reference-count
 * in the global file table. If there are no references anymore it releases the slot.
 *
 * @param pid the process-id
 * @param file the file
 */
void vfs_closeFile(tPid pid,tFileNo file);

/**
 * Creates a link @ <newPath> to <oldPath>
 *
 * @param pid the process-id
 * @param oldPath the link-target
 * @param newPath the link-name
 * @return 0 on success
 */
int vfs_link(tPid pid,const char *oldPath,const char *newPath);

/**
 * Removes the given file
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_unlink(tPid pid,const char *path);

/**
 * Creates the directory <path>
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_mkdir(tPid pid,const char *path);

/**
 * Removes the given directory
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
int vfs_rmdir(tPid pid,const char *path);

/**
 * Creates a driver-node for the given process and given name and opens a file for it
 *
 * @param pid the process-id
 * @param name the driver-name
 * @param flags the specified flags (implemented functions)
 * @return the file-number if ok, negative if an error occurred
 */
tFileNo vfs_createDriver(tPid pid,const char *name,uint flags);

/**
 * Checks wether the file has a receivable message
 *
 * @param pid the process-id
 * @param file the file
 * @return true if so
 */
bool vfs_hasMsg(tPid pid,tFileNo file);

/**
 * Checks wether the file can be read (driver has data to read)
 *
 * @param pid the process-id
 * @param file the file
 * @return true if so
 */
bool vfs_hasData(tPid pid,tFileNo file);

/**
 * For drivers: Looks whether a client wants to be served and return the node-number
 *
 * @param pid the driver-process-id
 * @param files an array of files to check for clients
 * @param count the number of files
 * @param index will be set to the index in <files> from which the client was chosen
 * @return the error-code or the node-number of the client
 */
tFileNo vfs_getClient(tPid pid,const tFileNo *files,size_t count,size_t *index);

/***
 * Fetches the client-id from the given file
 *
 * @param pid the process-id (driver-owner)
 * @param file the file for the client
 * @return the client-id or the error-code
 */
tInodeNo vfs_getClientId(tPid pid,tFileNo file);

/**
 * Opens a file for the client with given process-id.
 *
 * @param pid the own process-id
 * @param file the file for the driver
 * @param clientId the id of the desired client
 * @return the file or a negative error-code
 */
tFileNo vfs_openClient(tPid pid,tFileNo file,tInodeNo clientId);

/**
 * Creates a process-node with given pid and handler-function
 *
 * @param pid the process-id
 * @param handler the read-handler
 * @return the process-directory-node on success
 */
sVFSNode *vfs_createProcess(tPid pid,fRead handler);

/**
 * Removes all occurrences of the given process from VFS
 *
 * @param pid the process-id
 */
void vfs_removeProcess(tPid pid);

/**
 * Creates the VFS-node for the given thread
 *
 * @param tid the thread-id
 * @return true on success
 */
bool vfs_createThread(tTid tid);

/**
 * Removes all occurrences of the given thread from VFS
 *
 * @param tid the thread-id
 */
void vfs_removeThread(tTid tid);

#if DEBUGGING

/**
 * Prints all used entries in the global file table
 */
void vfs_dbg_printGFT(void);

/**
 * Prints all messages of all drivers
 */
void vfs_dbg_printMsgs(void);

/**
 * @return the number of entries in the global file table
 */
size_t vfs_dbg_getGFTEntryCount(void);

#endif

#endif /* VFS_H_ */
