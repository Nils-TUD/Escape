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
#include <sys/vfs/vfs.h>
#include <esc/fsinterface.h>

class VFSNode {
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
	static bool isValid(inode_t nodeNo);

	/**
	 * Requests the node with given number, i.e. locks it.
	 *
	 * @param nodeNo the node-number
	 * @return the node
	 */
	static VFSNode *request(inode_t nodeNo);

	/**
	 * Releases the given node, i.e. unlocks it.
	 *
	 * @param node the node
	 */
	static void release(VFSNode *node);

	/**
	 * @param nodeNo the node-number
	 * @return the node for given index
	 */
	static VFSNode *get(inode_t nodeNo);

	/**
	 * Prints the VFS tree
	 *
	 * @param os the output-stream
	 */
	static void printTree(OStream &os);

	/**
	 * Generates a unique id for given pid
	 *
	 * @param pid the process-id
	 * @return the name, allocated on the heap or NULL
	 */
	static char *generateId(pid_t pid);

	/**
	 * @param node the node
	 * @return the node-number of the given node
	 */
	inode_t getNo() const;

	/**
	 * 'Opens' the given directory, i.e. locks it, and returns the first child.
	 *
	 * @param dir the directory
	 * @param locked whether the directory should be locked
	 * @param isValid will be set to true or false depending on whether the node is still valid
	 * @return the first child-node
	 */
	sVFSNode *openDir(sVFSNode *dir,bool locked,bool *isValid);

	/**
	 * 'Closes' the given directory, i.e. releases the lock.
	 *
	 * @param dir the directory
	 * @param locked whether the directory is locked
	 */
	void closeDir(sVFSNode *dir,bool locked);

	/**
	 * @param node the node (is assumed to be locked)
	 * @return 0 if the given node is a dir and empty
	 */
	int isEmptyDir(sVFSNode *node);

	/**
	 * Determines the path for the given node. Note that static memory will be used for that!
	 * So you have to copy the path to another location if you want to keep the path.
	 *
	 * @param nodeNo the node-number
	 * @return the path
	 */
	const char *getPath(inode_t nodeNo);

	/**
	 * Retrieves information about the given node
	 *
	 * @param pid the process-id
	 * @param nodeNo the node-number
	 * @param info will be filled
	 * @return 0 on success
	 */
	int getInfo(pid_t pid,inode_t nodeNo,sFileInfo *info);

	/**
	 * Changes the mode of the given node
	 *
	 * @param pid the process-id
	 * @param nodeNo the node-number
	 * @param mode the new mode
	 * @return 0 on success
	 */
	int chmod(pid_t pid,inode_t nodeNo,mode_t mode);

	/**
	 * Changes the owner and group of the given node
	 *
	 * @param pid the process-id
	 * @param nodeNo the node-number
	 * @param uid the new user
	 * @param gid the new group
	 * @return 0 on success
	 */
	int chown(pid_t pid,inode_t nodeNo,uid_t uid,gid_t gid);

	/**
	 * Resolves the given path to a VFS-node
	 *
	 * @param path the path to resolve
	 * @param nodeNo the node-number for that path (will be set)
	 * @param created will be set to true if the (virtual) node didn't exist and has been created (may
	 * 	be NULL if you don't care about it)
	 * @param flags the flags (VFS_*) with which to resolve the path (create file,...)
	 * @return 0 if successfull or the error-code
	 */
	int resolvePath(const char *path,inode_t *nodeNo,bool *created,uint flags);

	/**
	 * Removes the last '/' from the path, if necessary, and returns a pointer to the last
	 * component of the path.
	 *
	 * @param path the path
	 * @param len the length of the path (will be updated)
	 * @return pointer to the last component
	 */
	char *basename(char *path,size_t *len);

	/**
	 * Removes the last component of the path
	 *
	 * @param path the path
	 * @param len the length of the path
	 */
	void dirname(char *path,size_t len);

	/**
	 * Finds the child-node with name <name>
	 *
	 * @param dir the directory (locked)
	 * @param name the name
	 * @param nameLen the length of the name
	 * @return the node or NULL
	 */
	sVFSNode *findInDir(sVFSNode *dir,const char *name,size_t nameLen);

	/**
	 * Finds the child-node with name <name>
	 *
	 * @param nodeNo the node-number of the directory
	 * @param name the name
	 * @param nameLen the length of the name
	 * @return the node or NULL
	 */
	sVFSNode *findInDirOf(inode_t nodeNo,const char *name,size_t nameLen);

	/**
	 * Creates a (incomplete) node without appending it
	 *
	 * @param pid the process-id
	 * @param name the node-name
	 * @return the node
	 */
	sVFSNode *create(pid_t pid,char *name);

	/**
	 * Appends the given node to <parent>.
	 *
	 * @param parent the parent-node (locked)
	 * @param node the node to append
	 */
	void append(sVFSNode *parent,sVFSNode *node);

	/**
	 * Decrements the references of the given node including all child-nodes. Nodes that have no refs
	 * anymore, are destroyed and released.
	 *
	 * @param n the node
	 */
	void destroy(sVFSNode *n);

	/**
	 * Decrements the references of the given node including all child-nodes and removes them from the
	 * VFS. They are only destroyed and released if there are no references anymore.
	 *
	 * @param n the node
	 */
	void destroyNow(sVFSNode *n);

	/**
	 * Prints the given VFS node
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;


	/**
	 * @param pid the process-id
	 * @return the size of the file
	 */
	virtual size_t getSize(pid_t pid) = 0;

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
	virtual ssize_t read(pid_t pid,OpenFile *file,void *buffer,off_t offset,size_t count) = 0;

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
	virtual ssize_t write(pid_t pid,OpenFile *file,const void *buffer,off_t offset,size_t count) = 0;

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
	virtual off_t seek(pid_t pid,off_t position,off_t offset,uint whence) = 0;

	/**
	 * Closes this file
	 *
	 * @param pid the process-id
	 * @param file the open-file
	 */
	virtual void close(pid_t pid,OpenFile *file) = 0;

	/**
	 * Destroys this node
	 */
	virtual void destroy() = 0;

private:
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
	VFSNode *const parent;
	VFSNode *prev;
	VFSNode *next;
	VFSNode *firstChild;
	VFSNode *lastChild;
	/* data in various forms, depending on the node-type */
	void *data;
};

