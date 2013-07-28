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

class Lock {
	Lock() = delete;

	/* a lock-entry */
	struct Entry {
		ulong ident;
		ushort flags;
		pid_t pid;
		volatile ushort readRefs;
		volatile tid_t writer;
		ushort waitCount;
	};

	static const ushort USED		= 4;

public:
	static const ushort EXCLUSIVE	= 1;
	static const ushort KEEP		= 2;

	/**
	 * Aquires the lock with given pid and ident. You can specify with the flags whether it should
	 * be an exclusive lock and whether it should be free'd if it is no longer needed (no waits atm)
	 *
	 * @param pid the process-id (INVALID_PID to lock it globally)
	 * @param ident to identify the lock
	 * @param flags flags (LOCK_*)
	 * @return 0 on success
	 */
	static int acquire(pid_t pid,ulong ident,ushort flags);

	/**
	 * Releases the lock with given ident and pid
	 *
	 * @param pid the process-id (INVALID_PID to lock it globally)
	 * @param ident to identify the lock
	 * @return 0 on success
	 */
	static int release(pid_t pid,ulong ident);

	/**
	 * Releases all locks of the given process
	 *
	 * @param pid the process-id
	 */
	static void releaseAll(pid_t pid);

	/**
	 * Prints all locks
	 */
	static void print();

private:
	/**
	 * Checks whether l is locked, depending on the flags
	 */
	static bool isLocked(const Entry *l,ushort flags);
	/**
	 * Searches the lock-entry for the given ident and process-id
	 */
	static ssize_t get(pid_t pid,ulong ident,bool free);

	static size_t lockCount;
	static Entry *locks;
	/* I think, in this case its better to use a single global lock instead of locking an sLock
	 * structure individually. Because when we're searching for a lock, we would have to do a lot
	 * of acquires and releases. Additionally, this module isn't used that extensively, so that it
	 * doesn't hurt to reduce the amount of parallelity a bit, IMO. */
	static klock_t klock;
};
