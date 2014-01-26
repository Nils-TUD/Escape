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
#include <sys/col/treap.h>
#include <sys/mem/dynarray.h>
#include <sys/vfs/node.h>
#include <sys/vfs/fileid.h>
#include <sys/semaphore.h>
#include <errno.h>
#include <assert.h>

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
	VFS_SEM = 64,			/* use semaphore operations */
	VFS_EXEC = 64,			/* kernel-intern: for accessing directories; the same as VFS_MSGS
							 * on purpose */
	VFS_EXCLUSIVE = 128,	/* disallow other accesses */
	VFS_NOLINKRES = 256,	/* kernel-intern: don't resolve last link in path */
	VFS_DEVICE = 512,		/* kernel-intern: whether the file was created for a device */
	VFS_NONODERES = 1024	/* kernel-intern: whether to use VFSNode::resolve in VFS::request */
};

class VFS;
class VMTree;
class FileDesc;
class ThreadBase;
class MountSpace;

/* an entry in the global file table */
class OpenFile {
	friend class VFS;
	friend class VMTree;
	friend class FileDesc;
	friend class ThreadBase;
	friend class MountSpace;

	struct SemTreapNode : public TreapNode<FileId> {
		explicit SemTreapNode(const FileId &id) : TreapNode<FileId>(id), sem() {
		}

	    virtual void print(OStream &os) {
			os.writef("f=(%d,%d) sem.value=%d\n",key().dev,key().ino,sem.getValue());
	    }

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
	 * @param fd will be set to the file-descriptor for the channel to receive a message from
	 * @param flags the flags (GW_*)
	 * @return the error-code or 0
	 */
	static int getWork(OpenFile *file,int *fd,uint flags);

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
	inode_t getNodeNo() const {
		return nodeNo;
	}
	/**
	 * @return the node of this file (might be NULL)
	 */
	VFSNode *getNode() const {
		return node;
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
	 * @param pid the process-id
	 * @param cmd the command (F_GETFL or F_SETFL)
	 * @param arg the argument (just used for F_SETFL)
	 * @return >= 0 on success
	 */
	int fcntl(pid_t pid,uint cmd,int arg);

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
	 * @param pid the process-id
	 * @param info the info to fill
	 * @return 0 on success
	 */
	int fstat(pid_t pid,sFileInfo *info) const;

	/**
	 * Sets the position for this file
	 *
	 * @param pid the process-id
	 * @param offset the offset
	 * @param whence the seek-type
	 * @return the new position on success
	 */
	off_t seek(pid_t pid,off_t offset,uint whence);

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
	 * @param forceBlock if set, the function will block regardless of the NOBLOCK-flag in file
	 * @return the number of written bytes (or < 0 if an error occurred)
	 */
	ssize_t receiveMsg(pid_t pid,msgid_t *id,void *data,size_t size,bool forceBlock);

	/**
	 * Shares a file with the device that hosts this channel.
	 *
	 * @param pid the process-id
	 * @param path the path to the file
	 * @param cliaddr the address where the client of this channel has mapped it
	 * @param size the size of the mmap'd file
	 * @return 0 on success
	 */
	int sharefile(pid_t pid,const char *path,void *cliaddr,size_t size);

	/**
	 * Writes all cached blocks of the affected filesystem to disk.
	 *
	 * @param pid the process-id
	 */
	int syncfs(pid_t pid);

	/**
	 * Closes this file. That means it calls Proc::closeFile() and decrements the reference-count
	 * in the global file table. If there are no references anymore it releases the slot.
	 *
	 * @param pid the process-id
	 * @return true if the file has really been closed
	 */
	bool close(pid_t pid);

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
		SpinLock::acquire(&lock);
		refCount++;
		SpinLock::release(&lock);
	}
	/**
	 * Increments the number of usages of this file
	 */
	void incUsages() {
		SpinLock::acquire(&lock);
		usageCount++;
		SpinLock::release(&lock);
	}
	/**
	 * Decrements the number of usages of this file
	 */
	void decUsages();

	/**
	 * Requests a new file or reuses an existing file for the given node+dev to open.
	 *
	 * @param pid the process-id
	 * @param flags the flags
	 * @param nodeNo the node-number
	 * @param devNo the dev-number
	 * @param n the VFS-node or NULL
	 * @param f will point to the used file afterwards
	 * @return 0 on success
	 */
	static int getFree(pid_t pid,ushort flags,inode_t nodeNo,dev_t devNo,const VFSNode *n,OpenFile **f);

	static void releaseFile(OpenFile *file);
	bool doClose(pid_t pid);

	klock_t lock;
	/* read OR write; flags = 0 => entry unused */
	ushort flags;
	/* the owner of this file */
	pid_t owner;
	/* number of references (file-descriptors) */
	ushort refCount;
	/* number of threads that are currently using this file (reading, writing, ...) */
	ushort usageCount;
	/* current position in file */
	off_t position;
	/* node-number */
	inode_t nodeNo;
	/* the node, if devNo == VFS_DEV_NO, the channel-node to fs for otherwise */
	VFSNode *node;
	/* the device-number */
	dev_t devNo;
	/* for real files: the path; for virt files: NULL */
	char *path;
	SemTreapNode *sem;
	/* for the freelist */
	OpenFile *next;

	/* global file table (expands dynamically) */
	static klock_t gftLock;
	static DynArray gftArray;
	static OpenFile *usedList;
	static OpenFile *exclList;
	static OpenFile *gftFreeList;
	static klock_t semLock;
	static Treap<SemTreapNode> sems;
};
