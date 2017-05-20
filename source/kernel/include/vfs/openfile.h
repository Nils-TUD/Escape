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

#include <esc/col/treap.h>
#include <fs/common.h>
#include <mem/dynarray.h>
#include <vfs/fileid.h>
#include <vfs/node.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <semaphore.h>
#include <string.h>

/* GFT flags */
enum {
	VFS_NOACCESS 	= 0,		/* no read and write */
	VFS_MSGS 		= 1,		/* exchange msgs with a device */
	VFS_EXEC		= 1,		/* kernel-intern: for accessing directories; the same as VFS_MSGS
							 	 * on purpose */
	VFS_WRITE 		= 2,
	VFS_READ 		= 4,
	VFS_CREATE 		= 8,
	VFS_TRUNCATE 	= 16,
	VFS_APPEND 		= 32,
	VFS_NOBLOCK 	= 64,
	VFS_LONELY 		= 128,		/* disallow other accesses */
	VFS_EXCL 		= 256,		/* fail if the file already exists */
	VFS_NOCHAN 		= 512,		/* don't create a channel, but open the device itself */
	VFS_NOFOLLOW	= 1024,		/* don't resolve last symlink */
	VFS_SYMLINK		= 2048,		/* kernel-intern: create symlink, if CREAT is specified */
	VFS_DEVICE 		= 4096,		/* kernel-intern: whether the file was created for a device */
	VFS_SIGNALS 	= 8192,		/* kernel-intern: allow signals during blocking */
	VFS_BLOCK 		= 16384,	/* kernel-intern: force blocking */

	/* all flags that the user can use */
	VFS_USER_FLAGS	= VFS_MSGS | VFS_WRITE | VFS_READ | VFS_CREATE | VFS_TRUNCATE |
						VFS_APPEND | VFS_NOBLOCK | VFS_LONELY | VFS_EXCL | VFS_NOCHAN |
						VFS_NOFOLLOW,
};

class VFS;
class VMTree;
class FileDesc;
class ThreadBase;
class MSTreeItem;
class MntSpace;

/* an entry in the global file table */
class OpenFile {
	friend class VFS;
	friend class VMTree;
	friend class FileDesc;
	friend class ThreadBase;
	friend class MSTreeItem;
	friend class MntSpace;

	struct SemTreapNode : public esc::TreapNode<FileId> {
		explicit SemTreapNode(const FileId &id) : esc::TreapNode<FileId>(id), refs(0), sem() {
		}

	    virtual void print(OStream &os) {
			os.writef("f=(%d,%d) refs=%d sem.value=%d\n",key().dev,key().ino,refs,sem.getValue());
	    }

	    int refs;
		Semaphore sem;
	};

	OpenFile() = delete;

public:
	/**
	 * @return the number of entries in the global file table
	 */
	static size_t getCount();

	/**
	 * For devices: Looks whether a client wants to be served. If necessary and if GW_NOBLOCK is
	 * not used, the function waits until a client wants to be served.
	 *
	 * @param file the file to check for a client
	 * @param flags the flags (GW_*)
	 * @return the error-code or the file descriptor
	 */
	static int getWork(OpenFile *file,uint flags);

	/**
	 * Checks whether they point to the same file
	 *
	 * @param f the second file
	 * @return true if device and node number are the same
	 */
	bool isSameFile(OpenFile *f) const {
		return devNo == f->devNo && nodeNo == f->nodeNo;
	}

	/**
	 * @return the path with which this file has been opened
	 */
	const char *getPath() const {
		if(path) {
			assert(devNo != VFS_DEV_NO);
			return path;
		}
		assert(devNo == VFS_DEV_NO);
		return node->getPath();
	}
	/**
	 * Copies the path into <dst>.
	 *
	 * @param dst the destination
	 * @param size the size of <dst>
	 */
	void getPathTo(char *dst,size_t size) const {
		if(path) {
			assert(devNo != VFS_DEV_NO);
			strnzcpy(dst,path,size);
		}
		else {
			assert(devNo == VFS_DEV_NO);
			node->getPathTo(dst,size);
		}
	}
	/**
	 * @return the device number
	 */
	dev_t getDev() const {
		return devNo;
	}
	/***
	 * @return the node-number
	 */
	ino_t getNodeNo() const {
		return nodeNo;
	}
	/**
	 * @return the node of this file (might be NULL)
	 */
	VFSNode *getNode() const {
		return node;
	}
	/**
	 * @return the flags
	 */
	ushort getFlags() const {
		return flags;
	}
	/**
	 * @return the mount permissions
	 */
	uint8_t getMntPerms() const {
		return mntperm;
	}
	/**
	 * @return the user with which the file has been opened
	 */
	const fs::User &getUser() const {
		return user;
	}
	/**
	 * @return true if the file was created for a device (and not a client of the device)
	 */
	bool isDevice() const {
		return flags & VFS_DEVICE;
	}
	/**
	 * @return whether the file should be used in blocking-mode
	 */
	bool shouldBlock() const {
		return !(flags & VFS_NOBLOCK);
	}

	/**
	 * Manipulates this file, depending on the command
	 *
	 * @param cmd the command (F_GETFL or F_SETFL)
	 * @param arg the argument (just used for F_SETFL)
	 * @return >= 0 on success
	 */
	int fcntl(uint cmd,int arg);

	/**
	 * Returns the current file-position
	 *
	 * @param pid the process-id
	 * @return the current file-position
	 */
	off_t tell(A_UNUSED pid_t pid) const {
		return position;
	}

	/**
	 * Retrieves information about this file
	 *
	 * @param info the info to fill
	 * @return 0 on success
	 */
	int fstat(struct stat *info) const;

	/**
	 * Sets the permissions of this file to <mode>.
	 *
	 * @param mode the new mode
	 * @return 0 on success
	 */
	int chmod(mode_t mode);

	/**
	 * Sets the owner and group of this file.
	 *
	 * @param uid the new user-id
	 * @param gid the new group-id
	 * @return 0 on success
	 */
	int chown(uid_t uid,gid_t gid);

	/**
	 * Sets the access and modification times of this file.
	 *
	 * @param utimes the new access and modification times
	 * @return 0 on success
	 */
	int utime(const struct utimbuf *utimes);

	/**
	 * Creates a link @ <dir>/<name> to <this>
	 *
	 * @param dir the directory
	 * @param name the name of the link to create
	 * @return 0 on success
	 */
	int link(OpenFile *dir,const char *name);

	/**
	 * Unlinks <this>/<name>
	 *
	 * @param name the name of the file to remove
	 * @return 0 on success
	 */
	int unlink(const char *name);

	/**
	 * Renames <this>/<oldName> to <newDir>/<newName>.
	 *
	 * @param oldName the name of the old file
	 * @param newDir the new directory
	 * @param newName the name of the file to create
	 * @return 0 on success
	 */
	int rename(const char *oldName,OpenFile *newDir,const char *newName);

	/**
	 * Creates the directory named <name> in this directory.
	 *
	 * @param name the directory path
	 * @param mode the mode to set
	 * @return 0 on success
	 */
	int mkdir(const char *name,mode_t mode);

	/**
	 * Removes the directory named <name> in this directory.
	 *
	 * @param name the filename
	 * @return 0 on success
	 */
	int rmdir(const char *name);

	/**
	 * Creates a symlink named <name> in this directory, pointing to <target>.
	 *
	 * @param name the filename
	 * @param target the target path
	 * @return 0 on success
	 */
	int symlink(const char *name,const char *target);

	/**
	 * Creates a device-node for the given process at <this>/<name> and opens a file for it.
	 *
	 * @param name the name of the device
	 * @param mode the mode to set
	 * @param type the device-type (DEV_TYPE_*)
	 * @param ops the supported operations
	 * @param file will be set to opened file
	 * @return 0 if ok, negative if an error occurred
	 */
	int createdev(const char *name,mode_t mode,uint type,uint ops,OpenFile **file);

	/**
	 * Creates a new channel for <file>.
	 *
	 * @param file the device
	 * @param perm the permissions
	 * @param chan will be set to the created channel
	 * @return 0 on success
	 */
	int createchan(uint perm,OpenFile **chan);

	/**
	 * Sets the position for this file
	 *
	 * @param offset the offset
	 * @param whence the seek-type
	 * @return the new position on success
	 */
	off_t seek(off_t offset,uint whence);

	/**
	 * Reads max. count bytes from this file into the given buffer and returns the number
	 * of read bytes.
	 *
	 * @param pid will be used to check whether the device writes or a device-user
	 * @param buffer the buffer to write to
	 * @param count the max. number of bytes to read
	 * @return the number of bytes read
	 */
	ssize_t read(pid_t pid,void *buffer,size_t count);

	/**
	 * Writes count bytes from the given buffer into this file and returns the number of written
	 * bytes.
	 *
	 * @param pid will be used to check whether the device writes or a device-user
	 * @param buffer the buffer to read from
	 * @param count the number of bytes to write
	 * @return the number of bytes written
	 */
	ssize_t write(pid_t pid,const void *buffer,size_t count);

	/**
	 * Sends a message to the corresponding device
	 *
	 * @param pid the sender-process-id
	 * @param id the message-id
	 * @param data1 the message-data
	 * @param size1 the data-size
	 * @param data2 for the device-messages: a second message (NULL = no second one)
	 * @param size2 the size of the second message
	 * @return 0 on success
	 */
	ssize_t sendMsg(pid_t pid,msgid_t id,USER const void *data1,size_t size1,
			USER const void *data2,size_t size2);

	/**
	 * Receives a message from the corresponding device
	 *
	 * @param pid the receiver-process-id
	 * @param id will be set to the fetched msg-id
	 * @param data the message to write to
	 * @param flags additional flags (overwrite flags in the file)
	 * @return the number of written bytes (or < 0 if an error occurred)
	 */
	ssize_t receiveMsg(pid_t pid,msgid_t *id,void *data,size_t size,uint flags);

	/**
	 * Truncates the file to <length> bytes by either extending it with 0-bytes or cutting it to
	 * that length.
	 *
	 * @param pid the receiver-process-id
	 * @param length the length of the file
	 * @return 0 on success
	 */
	int truncate(off_t length);

	/**
	 * Cancels the message <mid> that is currently in flight. If the device supports it, it waits
	 * until it has received the response. This tells us whether the message has been canceled or if
	 * the response has already been sent.
	 *
	 * @param pid the process-id
	 * @param mid the message-id to cancel
	 * @return 0 if it has been canceled, 1 if the reply is already available or < 0 on errors
	 */
	int cancel(pid_t pid,msgid_t mid);

	/**
	 * Delegates <file> to the driver over this channel.
	 *
	 * @param pid the process-id
	 * @param file the file to delegate
	 * @param perm the permissions to set for the file
	 * @param arg the argument to send to the driver
	 * @return 0 on success
	 */
	int delegate(pid_t pid,OpenFile *file,uint perm,int arg);

	/**
	 * Obtains a file from the driver over this channel.
	 *
	 * @param pid the process-id
	 * @param arg the argument to send to the driver
	 * @return the file descriptor for the obtained file on success
	 */
	int obtain(pid_t pid,int arg);

	/**
	 * Binds this file to the given thread.
	 *
	 * @param tid the thread-id
	 * @return 0 on success
	 */
	int bindto(tid_t tid);

	/**
	 * Writes all cached blocks of the affected filesystem to disk.
	 */
	int syncfs();

	/**
	 * Closes this file. That means it decrements the reference-count in the global file table. If
	 * there are no references anymore it releases the slot.
	 *
	 * @return true if the file has really been closed
	 */
	bool close();

	/**
	 * Prints information to this file
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;

	/**
	 * Prints all used entries in the global file table
	 *
	 * @param os the output-stream
	 */
	static void printAll(OStream &os);

private:
	/**
	 * Sets the path of this file
	 *
	 * @param path the new value
	 */
	void setPath(char *path) {
		this->path = path;
	}

	/**
	 * Increases the references of this file
	 */
	void incRefs() {
		LockGuard<SpinLock> g(&lock);
		refCount++;
	}
	/**
	 * Increments the number of usages of this file
	 */
	void incUsages() {
		LockGuard<SpinLock> g(&lock);
		usageCount++;
	}
	/**
	 * Decrements the number of usages of this file
	 */
	void decUsages();

	/**
	 * Requests a new file or reuses an existing file for the given node+dev to open.
	 *
	 * @param u the user
	 * @param mntperm the permissions for the mountpoint
	 * @param flags the flags
	 * @param nodeNo the node-number
	 * @param devNo the dev-number
	 * @param n the VFS-node or NULL
	 * @param f will point to the used file afterwards
	 * @param clone whether this is a clone of an existing OpenFile (no exclusive/lonely check)
	 * @return 0 on success
	 */
	static int getFree(const fs::User &u,uint8_t mntperm,ushort flags,ino_t nodeNo,dev_t devNo,
		const VFSNode *n,OpenFile **f,bool clone);

	static void releaseFile(OpenFile *file);
	bool doClose();

	SpinLock lock;
	/* the permissions for the mountpoint */
	uint8_t mntperm;
	/* read OR write; flags = 0 => entry unused */
	ushort flags;
	/* number of references (file-descriptors) */
	ushort refCount;
	/* number of threads that are currently using this file (reading, writing, ...) */
	ushort usageCount;
	/* current position in file */
	off_t position;
	/* node-number */
	ino_t nodeNo;
	/* the node, if devNo == VFS_DEV_NO, the channel-node to fs for otherwise */
	VFSNode *node;
	/* the device-number */
	dev_t devNo;
	/* the user and groups (for meta operations) */
	fs::User user;
	/* for real files: the path; for virt files: NULL */
	char *path;
	SemTreapNode *sem;
	/* for the freelist */
	OpenFile *next;

	/* global file table (expands dynamically) */
	static SpinLock gftLock;
	static DynArray gftArray;
	static OpenFile *usedList;
	static OpenFile *exclList;
	static OpenFile *gftFreeList;
	static SpinLock semLock;
	static esc::Treap<SemTreapNode> sems;
};
