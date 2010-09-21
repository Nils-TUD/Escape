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
#define MODE_TYPE_DRVUSE			0x0010000
#define MODE_TYPE_DRIVER			0x0020000
#define MODE_TYPE_PIPECON			0x0040000
#define MODE_TYPE_PIPE				0x0080000

/* the device-number of the VFS */
#define VFS_DEV_NO					((tDevNo)0xFF)

#define IS_DRIVER(mode)				(((mode) & MODE_TYPE_DRIVER) != 0)
#define IS_DRVUSE(mode)				(((mode) & MODE_TYPE_DRVUSE) != 0)
#define IS_PIPE(mode)				(((mode) & MODE_TYPE_PIPE) != 0)

#define DRV_OPEN					1
#define DRV_READ					2
#define DRV_WRITE					4
#define DRV_CLOSE					8
#define DRV_FS						16
#define DRV_TERM					32
#define DRV_IMPL(funcs,func)		(((funcs) & (func)) != 0)
#define DRV_IS_FS(funcs)			((funcs) == DRV_FS)

/* GFT flags */
enum {
	VFS_NOACCESS = 0,		/* no read and write */
	VFS_READ = 1,
	VFS_WRITE = 2,
	VFS_CREATE = 4,
	VFS_TRUNCATE = 8,
	VFS_APPEND = 16,
	VFS_CONNECT = 32,		/* kernel-intern: connect to driver */
	VFS_NOLINKRES = 64,		/* kernel-intern: don't resolve last link in path */
	VFS_CREATED = 128,		/* kernel-intern: whether a new node has been created */
	VFS_MODIFIED = 256		/* kernel-intern: whether it has been written to the file */
};

/* seek-types */
#define SEEK_SET					0
#define SEEK_CUR					1
#define SEEK_END					2

/* a node in our virtual file system */
typedef struct sVFSNode sVFSNode;
/* the function for read-requests on info-nodes */
typedef s32 (*fRead)(tPid pid,tFileNo file,sVFSNode *node,u8 *buffer,u32 offset,u32 count);
typedef s32 (*fWrite)(tPid pid,tFileNo file,sVFSNode *node,const u8 *buffer,u32 offset,u32 count);

struct sVFSNode {
	char *name;
	/* number of open files for this node */
	u8 refCount;
	/* 0 means unused; stores permissions and the type of node */
	u32 mode;
	/* for the vfs-structure */
	sVFSNode *parent;
	sVFSNode *prev;
	sVFSNode *next;
	sVFSNode *firstChild;
	sVFSNode *lastChild;
	/* the handler that perform a read-/write-operation on this node */
	fRead readHandler;
	fWrite writeHandler;
	/* the owner of this node: used for driver-usages */
	tPid owner;
	/* a list of listeners for created, modified or deleted */
	sSLList *listeners;
	union {
		/* for drivers/dev */
		struct {
			/* whether there is data to read or not */
			bool isEmpty;
			/* implemented functions */
			u32 funcs;
			/* the last served client */
			sVFSNode *lastClient;
		} driver;
		/* for driver-usages */
		struct {
			/* a list for sending messages to the driver */
			sSLList *sendList;
			/* a list for reading messages from the driver */
			sSLList *recvList;
		} drvuse;
		/* for pipes */
		struct {
			u32 total;
			sSLList *list;
		} pipe;
		/* for all other node-types */
		struct {
			/* size of the buffer */
			u32 size;
			/* currently used size */
			u32 pos;
			void *cache;
		} def;
	} data;
};

/**
 * Initializes the virtual file system
 */
void vfs_init(void);

/**
 * Checks whether the process with given pid has the permission to do the given stuff with <nodeNo>.
 *
 * @param pid the process-id
 * @param nodeNo the node-number
 * @param flags specifies what you want to do (VFS_READ | VFS_WRITE)
 * @return 0 if the process has permission or the error-code
 */
s32 vfs_hasAccess(tPid pid,tInodeNo nodeNo,u16 flags);

/**
 * Increases the references of the given file
 *
 * @param file the file
 * @return 0 on success
 */
s32 vfs_incRefs(tFileNo file);

/**
 * @param file the file-number
 * @return the owner-pid of the given file
 */
s32 vfs_getOwner(tFileNo file);

/**
 * Determines the node-number and device-number for the given file
 *
 * @param file the file
 * @param ino will be set to the inode-number
 * @param dev will be set to the devive-number
 * @return 0 on success
 */
s32 vfs_getFileId(tFileNo file,tInodeNo *ino,tDevNo *dev);

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
tFileNo vfs_openPath(tPid pid,u16 flags,const char *path);

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
tFileNo vfs_openFile(tPid pid,u16 flags,tInodeNo nodeNo,tDevNo devNo);

/**
 * Returns the current file-position
 *
 * @param pid the process-id
 * @param file the file
 * @return the current file-position
 */
u32 vfs_tell(tPid pid,tFileNo file);

/**
 * Checks whether we are at EOF in the given file
 *
 * @param pid the process-id
 * @param file the file
 * @return true if at EOF
 */
bool vfs_eof(tPid pid,tFileNo file);

/**
 * Retrieves information about the given path
 *
 * @param pid the process-id
 * @param path the path
 * @param info the info to fill
 * @return 0 on success
 */
s32 vfs_stat(tPid pid,const char *path,sFileInfo *info);

/**
 * Retrieves information about the given file
 *
 * @param pid the process-id
 * @param file the file
 * @param info the info to fill
 * @return 0 on success
 */
s32 vfs_fstat(tPid pid,tFileNo file,sFileInfo *info);

/**
 * Checks whether a message is available
 *
 * @param pid the process-id
 * @param file the file
 * @return 1 if so, 0 if not, < 0 if an error occurred
 */
s32 vfs_hasMsg(tPid pid,tFileNo file);

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
s32 vfs_seek(tPid pid,tFileNo file,s32 offset,u32 whence);

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
s32 vfs_readFile(tPid pid,tFileNo file,u8 *buffer,u32 count);

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
s32 vfs_writeFile(tPid pid,tFileNo file,const u8 *buffer,u32 count);

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
s32 vfs_sendMsg(tPid pid,tFileNo file,tMsgId id,const u8 *data,u32 size);

/**
 * Receives a message from the corresponding driver
 *
 * @param pid the receiver-process-id
 * @param file the file to receive the message from
 * @param id will be set to the fetched msg-id
 * @param data the message to write to
 * @return the number of written bytes (or < 0 if an error occurred)
 */
s32 vfs_receiveMsg(tPid pid,tFileNo file,tMsgId *id,u8 *data,u32 size);

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
s32 vfs_link(tPid pid,const char *oldPath,const char *newPath);

/**
 * Removes the given file
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
s32 vfs_unlink(tPid pid,const char *path);

/**
 * Creates the directory <path>
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
s32 vfs_mkdir(tPid pid,const char *path);

/**
 * Removes the given directory
 *
 * @param pid the process-id
 * @param path the path
 * @return 0 on success
 */
s32 vfs_rmdir(tPid pid,const char *path);

/**
 * Creates a driver-node for the given process and given name
 *
 * @param pid the process-id
 * @param name the driver-name
 * @param flags the specified flags (implemented functions)
 * @return 0 if ok, negative if an error occurred
 */
s32 vfs_createDriver(tPid pid,const char *name,u32 flags);

/**
 * Sets whether data is currently readable or not
 *
 * @param pid the process-id
 * @param nodeNo the driver-node-number
 * @param readable whether there is data or not
 * @return 0 on success
 */
s32 vfs_setDataReadable(tPid pid,tInodeNo nodeNo,bool readable);

/**
 * Checks whether there is a message for the given process. That if the process is a driver
 * and should serve a client or if the process has got a message from a driver.
 *
 * @param pid the process-id
 * @param events the events to wait for
 * @return true if there is a message
 */
bool vfs_msgAvailableFor(tPid pid,u32 events);

/**
 * Wakes up all clients of the given node for given events
 *
 * @param node the node of the driver
 * @param events the events
 */
void vfs_wakeupClients(sVFSNode *node,u32 events);

/**
 * For drivers: Looks whether a client wants to be served and return the node-number
 *
 * @param pid the driver-process-id
 * @param vfsNodes an array of VFS-nodes to check for clients
 * @param count the size of <sys/vfsNodes>
 * @return the error-code or the node-number of the client
 */
s32 vfs_getClient(tPid pid,tInodeNo *vfsNodes,u32 count);

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
 * @param nodeNo the driver-node-number
 * @param clientId the id of the desired client
 * @return the file or a negative error-code
 */
tFileNo vfs_openClient(tPid pid,tInodeNo nodeNo,tInodeNo clientId);

/**
 * Removes the driver with given node-number
 *
 * @param pid the process-id
 * @param nodeNo the node-number of the driver
 */
s32 vfs_removeDriver(tPid pid,tInodeNo nodeNo);

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
 * Prints the avaiable messages for the given driver-node
 */
void vfs_dbg_printMsgsOf(sVFSNode *n);

/**
 * @return the number of entries in the global file table
 */
u32 vfs_dbg_getGFTEntryCount(void);

#endif

#endif /* VFS_H_ */
