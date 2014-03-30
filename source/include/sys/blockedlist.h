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
#include <sys/col/slist.h>
#include <sys/task/sched.h>

class Thread;

/**
 * A list of waiting threads.
 */
class BlockedList {
public:
	explicit BlockedList() : list() {
	}

	size_t length() const {
		return list.length();
	}

	/**
	 * Blocks the given thread, which HAS TO be the current one. That is, it enqueues it to the
	 * waiting-threads-list and sets the thread-status to BLOCKED.
	 *
	 * @param t the thread
	 */
	void block(Thread *t);

	/**
	 * Removes the given thread from the list.
	 *
	 * @param t the thread
	 */
	void remove(Thread *t);

	/**
	 * Wakes up the next thread
	 */
	void wakeup();

	/**
	 * Wakes up all waiting threads
	 */
	void wakeupAll();

private:
	SList<Thread> list;
};
