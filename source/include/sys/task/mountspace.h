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

#include <sys/common.h>
#include <sys/col/pathtree.h>
#include <sys/col/slist.h>

class Proc;
class OStream;
class OpenFile;

/**
 * A mountspace is a list of mountpoints. Every process has a mountspace, which can be shared with
 * others. On fork(), the mountspace is shared with the parent, i.e. only the references are
 * increased. If the process desires, it can clone it's mountspace which gives him its own one.
 * Every mountspace has a unique id which can e.g. be given to other processes so that they can
 * join that mountspace.
 */
class MountSpace : public SListItem {
	explicit MountSpace() : SListItem(), id(++next_id), refs(1), tree() {
	}

public:
	/**
	 * @param p the process
	 * @return the id of the mountspace of given process
	 */
	static int getId(Proc *p);

	/**
	 * Resolves the given path for given process, i.e. it searches for the mountpoint in which
	 * the given path resides. If successfull, a reference will be added, which is why you have
	 * to call release() when you're done.
	 *
	 * @param p the process
	 * @param path the path to resolve
	 * @param begin will be set to the beginning of the path in the found mountpoint
	 * @param file will be set to the file associated with the found mountpoint
	 * @return 0 on success
	 */
	static int request(Proc *p,const char *path,const char **begin,OpenFile **file);

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
	static int mount(Proc *p,const char *path,OpenFile *file);

	/**
	 * Unmounts the filesystem that is mounted at <path>.
	 *
	 * @param p the process
	 * @param path the path
	 * @return 0 on success
	 */
	static int unmount(Proc *p,const char *path);

	/**
	 * Creates a new empty mountspace for the given process. This should only be used for the
	 * initial mountspace, since afterwards we only use clone.
	 *
	 * @param p the process
	 */
	static void create(Proc *p);

	/**
	 * Clones the mountspace of process <p>. That is, it creates a new mountspace for which mount
	 * and unmount operations don't affect others.
	 *
	 * @param p the process
	 * @return the new mountspace id on success
	 */
	static int clone(Proc *p);

	/**
	 * Inherits the mountspace of <src> to <dst>.
	 *
	 * @param dst the destination process
	 * @param src the source process
	 */
	static void inherit(Proc *dst,Proc *src);

	/**
	 * Leaves the current mountspace and joins the point denoted by <id>.
	 *
	 * @param p the process
	 * @param id the id of the mountspace to join
	 * @return 0 on success
	 */
	static int join(Proc *p,int id);

	/**
	 * Leaves the current mountspace (only possible for destroying processes, since it has no mount-
	 * space afterwards).
	 *
	 * @param p the process
	 */
	static void leave(Proc *p) {
		lock.down();
		doLeave(p);
		lock.up();
	}

	/**
	 * Prints the mountspace of the given process
	 *
	 * @param os the output-stream
	 * @param p the process
	 */
	static void print(OStream &os,Proc *p);

	/**
	 * Prints all mountspaces
	 *
	 * @param os the output-stream
	 */
	static void printAll(OStream &os);

private:
	static void doLeave(Proc *p);
	static void printItem(OStream &os,OpenFile *file);

	int id;
	ulong refs;
	PathTree<OpenFile> tree;

	static SpinLock lock;
	static int next_id;
	static SList<MountSpace> list;
};
