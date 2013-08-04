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
#include <sys/mem/dynarray.h>
#include <esc/fsinterface.h>
#include <errno.h>

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
#define IS_PIPE(mode)				(((mode) & MODE_TYPE_PIPE) != 0)
#define IS_FS(mode)					(((mode) & MODE_TYPE_DEVMASK) == 0x0300000)

class VFSDevice;
class VFSDir;
class VFSInfo;
class OpenFile;

class VFSNode {
	/* we do often handle with VFSNode objects and still want to have access to the protected
	 * members */
	friend class VFSDevice;
	friend class VFSDir;
	friend class VFSInfo;
	friend class OpenFile;

public:
	/**
	 * Initializes the nodes
	 */
	static void init();

	/**
	 * Checks whether the given node-number is valid
	 *
	 * @param nodeNo the number
	 * @return true if so
	 */
	static bool isValid(inode_t nodeNo) {
		return nodeNo >= 0 && nodeNo < (inode_t)nodeArray.getObjCount();
	}

	/**
	 * Acquires the tree-lock. While holding this lock, the whole VFS-tree-structure can't change
	 * including the node-names. This lock can NOT be grabbed while holding the node-lock!
	 */
	static void acquireTree() {
		SpinLock::acquire(&treeLock);
	}
	/**
	 * Releases the tree-lock.
	 */
	static void releaseTree() {
		SpinLock::release(&treeLock);
	}

	/**
	 * Resolves the given path to a VFS-node and requests it, i.e. increments the references.
	 * This way, you can be sure that if 0 is returned, the node is not destroyed until
	 * you call release(). On the other hand, it might get detached from the tree if you don't
	 * prevent that by acquiring the tree-lock.
	 *
	 * @param path the path to resolve
	 * @param node the node for that path (will be set)
	 * @param created will be set to true if the (virtual) node didn't exist and has been created (may
	 * 	be NULL if you don't care about it)
	 * @param flags the flags (VFS_*) with which to resolve the path (create file,...)
	 * @return 0 if successfull or the error-code
	 */
	static int request(const char *path,VFSNode **node,bool *created,uint flags);

	/**
	 * Releases the given node, i.e. unlocks it.
	 *
	 * @param node the node
	 */
	static void release(VFSNode *node) {
		if(node)
			node->unref();
	}

	/**
	 * @param nodeNo the node-number
	 * @return the node for given index
	 */
	static VFSNode *get(inode_t nodeNo) {
		return (VFSNode*)nodeArray.getObj(nodeNo);
	}

	/**
	 * @return the number of allocated nodes
	 */
	static size_t getNodeCount() {
		return allocated;
	}

	/**
	 * Prints the VFS tree
	 *
	 * @param os the output-stream
	 */
	static void printTree(OStream &os);

	/**
	 * Removes the last '/' from the path, if necessary, and returns a pointer to the last
	 * component of the path.
	 *
	 * @param path the path
	 * @param len the length of the path (will be updated)
	 * @return pointer to the last component
	 */
	static char *basename(char *path,size_t *len);

	/**
	 * Removes the last component of the path
	 *
	 * @param path the path
	 * @param len the length of the path
	 */
	static void dirname(char *path,size_t len);

	/**
	 * Creates a new node with given owner and name
	 *
	 * @param pid the owner
	 * @param name the node-name
	 * @param mode the mode
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSNode(pid_t pid,char *name,uint mode,bool &success);

	/**
	 * Destructor
	 */
	virtual ~VFSNode() {
	}

	/**
	 * @return whether this node is still alive, i.e. hasn't been detached yet
	 */
	bool isAlive() const {
		return name != NULL;
	}

	/**
	 * @param node the node
	 * @return the node-number of the given node
	 */
	inode_t getNo() const {
		return (inode_t)nodeArray.getIndex(this);
	}

	/**
	 * @return the name of the node (NULL if the node has been detached)
	 */
	const char *getName() const {
		return name;
	}
	/**
	 * @return the mode
	 */
	uint getMode() const {
		return mode;
	}
	/**
	 * @return the user-id
	 */
	uid_t getUid() const {
		return uid;
	}
	/**
	 * @return the group-id
	 */
	gid_t getGid() const {
		return gid;
	}
	/**
	 * @return the owner (creator) of this node
	 */
	pid_t getOwner() const {
		return owner;
	}
	/**
	 * @return the parent node
	 */
	VFSNode *getParent() {
		return parent;
	}
	const VFSNode *getParent() const {
		return parent;
	}
	/**
	 * @return the number of references (only for unit-tests!)
	 */
	ushort getRefCount() const {
		return refCount;
	}

	/**
	 * 'Opens' this directory, i.e. grabs the tree-lock and returns the first child. You can assume
	 * that the VFS-tree doesn't change in until closeDir() is called including the node-names.
	 *
	 * @param locked whether to grab the lock
	 * @param isValid will be set to true or false depending on whether the node is still valid
	 * @return the first child-node
	 */
	const VFSNode *openDir(bool locked,bool *isValid) const;

	/**
	 * 'Closes' this directory, i.e. releases the tree-lock.
	 *
	 * @param locked whether you grabbed the lock on open.
	 * @param dir the directory
	 */
	void closeDir(bool locked) const {
		if(locked)
			SpinLock::release(&treeLock);
	}

	/**
	 * @param node the node (is assumed to be locked)
	 * @return 0 if this node is a dir and empty
	 */
	int isEmptyDir() const;

	/**
	 * Retrieves information about this node
	 *
	 * @param pid the process-id
	 * @param info will be filled
	 */
	void getInfo(pid_t pid,sFileInfo *info) const;

	/**
	 * Determines the path for this node. Note that static memory will be used for that!
	 * So you have to copy the path to another location if you want to keep the path.
	 *
	 * @return the path
	 */
	const char *getPath() const;

	/**
	 * Changes the mode of this node
	 *
	 * @param pid the process-id
	 * @param mode the new mode
	 * @return 0 on success
	 */
	int chmod(pid_t pid,mode_t mode);

	/**
	 * Changes the owner and group of this node
	 *
	 * @param pid the process-id
	 * @param uid the new user
	 * @param gid the new group
	 * @return 0 on success
	 */
	int chown(pid_t pid,uid_t uid,gid_t gid);

	/**
	 * Finds the child-node with name <name>
	 *
	 * @param dir the directory (locked)
	 * @param name the name
	 * @param nameLen the length of the name
	 * @return the node or NULL
	 */
	const VFSNode *findInDir(const char *name,size_t nameLen) const;

	/**
	 * Decrements the references of this node including all child-nodes. Nodes that have no refs
	 * anymore, are destroyed and released.
	 */
	void unref() {
		doUnref(false);
	}

	/**
	 * Decrements the references of this node including all child-nodes and removes them from the
	 * VFS. They are only destroyed and released if there are no references anymore.
	 */
	void destroy() {
		doUnref(true);
	}

	/**
	 * Gives the implementation a chance to react on a open of this node.
	 *
	 * @param pid the process-id
	 * @param file the open-file
	 * @param flags the open-flags
	 */
	virtual ssize_t open(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,A_UNUSED uint flags) {
		return 0;
	}

	/**
	 * @param pid the process-id
	 * @return the size of the file
	 */
	virtual size_t getSize(A_UNUSED pid_t pid) const {
		return 0;
	}

	/**
	 * Reads <count> bytes at <offset> into <buffer>.
	 *
	 * @param pid the process-id
	 * @param file the open-file
	 * @param buffer the target
	 * @param offset the offset
	 * @param count the number of bytes
	 * @return the number of read bytes or a negative error-code
	 */
	virtual ssize_t read(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,A_UNUSED void *buffer,
	                     A_UNUSED off_t offset,A_UNUSED size_t count) {
		return -ENOTSUP;
	}

	/**
	 * Writes <count> bytes from <buffer> to <offset>.
	 *
	 * @param pid the process-id
	 * @param file the open-file
	 * @param buffer the source
	 * @param offset the offset
	 * @param count the number of bytes
	 * @return the number of written bytes or a negative error-code
	 */
	virtual ssize_t write(A_UNUSED pid_t pid,A_UNUSED OpenFile *file,A_UNUSED const void *buffer,
	                      A_UNUSED off_t offset,A_UNUSED size_t count) {
		return -ENOTSUP;
	}

	/**
	 * Determines the position to set if we are at <position> and want to do a seek to <offset> of
	 * type <whence>.
	 *
	 * @param pid the process-id
	 * @param position the current position
	 * @param the offset to set
	 * @param whence the type of seek
	 * @return the position to set
	 */
	virtual off_t seek(A_UNUSED pid_t pid,A_UNUSED off_t position,A_UNUSED off_t offset,
	                   A_UNUSED uint whence) {
		return -ENOTSUP;
	}

	/**
	 * Closes this file
	 *
	 * @param pid the process-id
	 * @param file the open-file
	 */
	virtual void close(A_UNUSED pid_t pid,A_UNUSED OpenFile *file) {
		unref();
	}

	/**
	 * Prints the given VFS node
	 *
	 * @param os the output-stream
	 */
	virtual void print(OStream &os) const;

	/**
	 * Overloaded new- and delete operators which use the dynamic array.
	 */
	static void *operator new(size_t size) throw();
	static void operator delete(void *ptr) throw();

protected:
	/**
	 * Invalidates the node.
	 */
	virtual void invalidate() {
	}

	/**
	 * Generates a unique id for given pid
	 *
	 * @param pid the process-id
	 * @return the name, allocated on the heap or NULL
	 */
	static char *generateId(pid_t pid);
	/**
	 * Appends this node to <parent>.
	 *
	 * @param parent the parent-node (locked)
	 */
	void append(VFSNode *parent);

	const VFSNode *increaseRefs() const {
		/* TODO use atomic ops */
		SpinLock::acquire(&lock);
		refCount++;
		SpinLock::release(&lock);
		return this;
	}

private:
	static int createFile(pid_t pid,const char *path,VFSNode *dir,VFSNode **child,bool *created);
	static void doPrintTree(OStream &os,size_t level,const VFSNode *parent);
	void doUnref(bool remove);

protected:
	mutable klock_t lock;
	const char *name;
	size_t nameLen;
	/* number of open files for this node */
	mutable ushort refCount;
	/* the owner of this node */
	pid_t owner;
	uid_t uid;
	gid_t gid;
	/* 0 means unused; stores permissions and the type of node */
	uint mode;
	/* for the vfs-structure */
	VFSNode *parent;
	VFSNode *prev;
	VFSNode *firstChild;
	VFSNode *lastChild;
public:
	VFSNode *next;

private:
	/* all nodes (expand dynamically) */
	static DynArray nodeArray;
	/* a pointer to the first free node (which points to the next and so on) */
	static VFSNode *freeList;
	static uint nextUsageId;
	static klock_t nodesLock;
	static klock_t treeLock;
	static size_t allocated;
};
