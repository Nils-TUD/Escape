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

#include <esc/pathtree.h>
#include <vfs/ms.h>
#include <vfs/openfile.h>
#include <common.h>

class Proc;

class MSTreeItem : public esc::PathTreeItem<OpenFile>, public CacheAllocatable {
public:
	explicit MSTreeItem(char *name,OpenFile *data = NULL)
		: esc::PathTreeItem<OpenFile>(name,data), CacheAllocatable(), root(0), devno(0) {
	}
	explicit MSTreeItem(const MSTreeItem &i);

	ino_t root;
	dev_t devno;
};

class MSPathTree : public esc::PathTree<OpenFile,MSTreeItem> {
public:
	void print(OStream &os) const {
		printRec(os,this->_root);
	}

private:
	void printRec(OStream &os,MSTreeItem *item) const;
};

/**
 * Represents a mountspace. Every process uses one of these objects as its mountspace. The reference
 * counter is used for the processes that link here. The file cannot be unlinked from userspace to
 * prevent that somebody removes the last reference in this way when one process is still using it.
 */
class MntSpace : public CacheAllocatable {
public:
	typedef uint64_t id_t;

private:
	static const size_t MAX_MNT_SPACES	= 32;

	explicit MntSpace(id_t id,const fs::User &u,VFSNode *parent,char *name);

public:
	static MntSpace *create(const fs::User &u,VFSNode *parent,char *name);
	static MntSpace *request(id_t id);
	static void release(MntSpace *ms);

	~MntSpace();

	/**
	 * Clones this mountspace into a new one.
	 *
	 * @param u the user
	 * @param name the name of the VFSNode
	 * @return the new mountspace
	 */
	MntSpace *clone(const fs::User &u,char *name);

	/**
	 * Resolves the given path for given process, i.e. it searches for the mountpoint in which
	 * the given path resides. If successfull, a reference will be added, which is why you have
	 * to call release() when you're done.
	 *
	 * @param path the path to resolve
	 * @param begin will be set to the beginning of the path in the found mountpoint
	 * @param file will be set to the file associated with the found mountpoint
	 * @return the root inode on success
	 */
	ino_t request(const char *path,const char **begin,OpenFile **file);

	/**
	 * Releases the previously with request() requested file.
	 *
	 * @param file the file
	 */
	static void release(OpenFile *file);

	/**
	 * @return the VFS node
	 */
	VFSNode *getNode() {
		return _node;
	}

	/**
	 * Mounts the filesystem, handled by <file>, at <path>.
	 *
	 * @param path the path to mount it at
	 * @param file the file that points to the channel over which the fs is handled
	 * @return 0 on success
	 */
	int mount(const char *path,OpenFile *file);

	/**
	 * Remounts <dir> with given permissions.
	 *
	 * @param u the user
	 * @param dir the directory to remount
	 * @param flags the flags (rwx) to use; only downgrading is allowed
	 * @return 0 on success
	 */
	int remount(const fs::User &u,OpenFile *dir,uint flags);

	/**
	 * Unmounts the filesystem that is mounted at <path>.
	 *
	 * @param path the path
	 * @return 0 on success
	 */
	int unmount(const char *path);

	/**
	 * Leaves the current mountspace and joins this.
	 *
	 * @param p the process
	 */
	void join(Proc *p);

	/**
	 * Leaves the current mountspace (only possible for destroying processes, since it has no mount-
	 * space afterwards).
	 *
	 * @param p the process
	 */
	void leave(Proc *p);

	/**
	 * Prints this mountspace to given stream
	 *
	 * @param os the output stream
	 */
	void print(OStream &os) const;

private:
	bool init();

	id_t _id;
	long _refs;
	mutable SpinLock _lock;
	MSPathTree _tree;
	VFSMS *_node;
	static id_t _nextId;
	static SpinLock _tableLock;
	static MntSpace *_table[MAX_MNT_SPACES];
};
