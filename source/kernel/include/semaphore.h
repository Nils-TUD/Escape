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

#include <blockedlist.h>
#include <common.h>
#include <spinlock.h>

/**
 * Base-class for the semaphore which is more flexible than the subclass since it doesn't use a
 * lock internally but one that the user of BaseSem specifies.
 */
class BaseSem {
public:
	explicit BaseSem(int value = 1) : value(value), waiters() {
	}

	/**
	 * @return the current value
	 */
	int getValue() const {
		return value;
	}

	/**
	 * Aquires this mutex. It won't use busy-waiting here, but suspend the thread when the mutex
	 * is not available. If required, it will release <lck>. But note that it will NOT grab the lock
	 * again after wakeup. So the caller needs to do that afterwards, if necessary.
	 *
	 * @param lck the lock to release when blocking
	 * @param allowSigs whether signals should interrupt the operation
	 * @return true if the semaphore has been acquired (= no signal interrupted us)
	 */
	bool down(SpinLock *lck,bool allowSigs = false);

	/**
	 * Tries to acquire this mutex. If its locked, it does not block, but return false.
	 *
	 * @return true if the mutex has been acquired
	 */
	bool tryDown() {
		bool res = false;
		if(value > 0 && waiters.length() == 0) {
			value--;
			res = true;
		}
		return res;
	}

	/**
	 * Releases this semaphore
	 */
	void up() {
		value++;
		waiters.wakeup();
	}

private:
	int value;
	BlockedList waiters;
};

/**
 * The commonly used and simpler semaphore with an own internal lock.
 */
class Semaphore : public BaseSem {
public:
	explicit Semaphore(int value = 1) : BaseSem(value), lck() {
	}

	bool down(bool allowSigs = false) {
		LockGuard<SpinLock> g(&lck);
		return BaseSem::down(&lck,allowSigs);
	}

	bool tryDown() {
		LockGuard<SpinLock> g(&lck);
		return BaseSem::tryDown();
	}

	void up() {
		LockGuard<SpinLock> g(&lck);
		BaseSem::up();
	}

private:
	SpinLock lck;
};
