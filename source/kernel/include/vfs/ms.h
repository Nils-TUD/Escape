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

#include <col/kpathtree.h>
#include <vfs/node.h>
#include <common.h>

/**
 * Represents a mountspace. Every process uses one of these objects as its mountspace. The reference
 * counter is used for the processes that link here. The file cannot be unlinked from userspace to
 * prevent that somebody removes the last reference in this way when one process is still using it.
 */
class VFSMS : public VFSNode {
	class MSTreeItem : public esc::PathTreeItem<OpenFile>, public CacheAllocatable {
	public:
		explicit MSTreeItem(char *name,OpenFile *data = NULL)
			: esc::PathTreeItem<OpenFile>(name,data), CacheAllocatable() {
		}
		explicit MSTreeItem(const MSTreeItem &i);
	};

public:
	/**
	 * Creates an empty mountspace file.
	 *
	 * @param pid the process-id to use
	 * @param parent the parent-node
	 * @param name the node-name
	 * @param mode the mode to set
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSMS(pid_t pid,VFSNode *parent,char *name,mode_t mode,bool &success)
		: VFSNode(pid,name,S_IFMS | (mode & MODE_PERM),success), _tree() {
		if(!success)
			return;

		/* auto-destroy if the last process stops using it */
		refCount--;
		append(parent);
	}

	/**
	 * Creates a mountspace file that clones <ms>.
	 *
	 * @param pid the process-id to use
	 * @param ms the mountspace to clone
	 * @param parent the parent-node
	 * @param name the node-name
	 * @param mode the mode to set
	 * @param success whether the constructor succeeded (is expected to be true before the call!)
	 */
	explicit VFSMS(pid_t pid,const VFSMS &ms,VFSNode *parent,char *name,mode_t mode,bool &success)
		: VFSNode(pid,name,S_IFMS | (mode & MODE_PERM),success), _tree() {
		if(!success)
			return;
		{
			LockGuard<SpinLock> guard(&ms.lock);
			if(_tree.replaceWith(ms._tree) != 0) {
				success = false;
				return;
			}
		}

		/* auto-destroy if the last process stops using it */
		refCount--;
		append(parent);
	}

	virtual bool isDeletable() const {
		return false;
	}

	/**
	 * Resolves the given path for given process, i.e. it searches for the mountpoint in which
	 * the given path resides. If successfull, a reference will be added, which is why you have
	 * to call release() when you're done.
	 *
	 * @param path the path to resolve
	 * @param begin will be set to the beginning of the path in the found mountpoint
	 * @param file will be set to the file associated with the found mountpoint
	 * @return 0 on success
	 */
	int request(const char *path,const char **begin,OpenFile **file);

	/**
	 * Releases the previously with request() requested file.
	 *
	 * @param file the file
	 */
	static void release(OpenFile *file);

	/**
	 * Mounts the filesystem, handled by <file>, at <path>.
	 *
	 * @param p the process
	 * @param path the path to mount it at
	 * @param file the file that points to the channel over which the fs is handled
	 * @return 0 on success
	 */
	int mount(Proc *p,const char *path,OpenFile *file);

	/**
	 * Unmounts the filesystem that is mounted at <path>.
	 *
	 * @param p the process
	 * @param path the path
	 * @return 0 on success
	 */
	int unmount(Proc *p,const char *path);

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

	virtual ssize_t read(pid_t pid,OpenFile *f,void *buffer,off_t offset,size_t count);
	virtual void print(OStream &os) const;

private:
	static void readCallback(VFSNode *node,size_t *dataSize,void **buffer);
	static void printItem(OStream &os,OpenFile *file);

	KPathTree<OpenFile,MSTreeItem> _tree;
};
