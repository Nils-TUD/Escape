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

#include <common.h>
#include <spinlock.h>

class Proc;
class OStream;

class Groups {
	Groups() = delete;

public:
	struct Entries {
		int refCount;
		size_t count;
		gid_t *groups;
	};

	/**
	 * Allocates a new group-descriptor with <groups> (<count> groups) and assigns it to the given
	 * process.
	 *
	 * @param pid the process-id
	 * @param count the number of groups
	 * @param groups the groups-array (will be copied)
	 * @return true if successfull
	 */
	static bool set(pid_t pid,size_t count,USER const gid_t *groups);

	/**
	 * Lets process <dst> join the groups of process <src>.
	 *
	 * @param dst the dest-process
	 * @param src the src-process
	 */
	static void join(Proc *dst,Proc *src);

	/**
	 * Copies the group-ids from the given process into the given list. If <count> is 0, the number
	 * of groups is returned
	 *
	 * @param pid the process-id
	 * @param list the destination list
	 * @param count the number of items in the list
	 * @return the number of copied groups
	 */
	static size_t get(pid_t pid,gid_t *list,size_t count);

	/**
	 * Checks whether the groups of process <p> contain <gid>.
	 *
	 * @param pid the process-id
	 * @param gid the group-id
	 * @return true if so
	 */
	static bool contains(pid_t pid,gid_t gid);

	/**
	 * Lets the given process leave its groups
	 *
	 * @param pid the process-id
	 */
	static void leave(pid_t pid);

	/**
	 * Prints the groups of the given process
	 *
	 * @param os the output-stream
	 * @param pid the process-id
	 */
	static void print(OStream &os,pid_t pid);

private:
	static SpinLock lock;
};
