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

class Semaphore {
public:
	explicit Semaphore(int value = 1) : value(value), lock() {
	}

	/**
	 * @return the current value
	 */
	int getValue() const {
		return value;
	}

	/**
	 * Aquires this mutex. It won't use busy-waiting here, but suspend the thread when the mutex
	 * is not available.
	 */
	void down();

	/**
	 * Tries to acquire this mutex. If its locked, it does not block, but return false.
	 *
	 * @return true if the mutex has been acquired
	 */
	bool tryDown();

	/**
	 * Releases this semaphore
	 */
	void up();

private:
	int value;
	klock_t lock;
};
