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

#include <task/thread.h>
#include <common.h>
#include <mutex.h>
#include <spinlock.h>
#include <util.h>

void Mutex::down() {
#if DEBUG_LOCKS
	if(Util::IsPanicStarted())
		return;
#endif

	Thread *t = Thread::getRunning();
	if(holder != t->getTid()) {
		Semaphore::down();
		assert(holder == INVALID && depth == 0);
		holder = t->getTid();
	}
	depth++;
}

bool Mutex::tryDown() {
#if DEBUG_LOCKS
	if(Util::IsPanicStarted())
		return true;
#endif

	Thread *t = Thread::getRunning();
	if(holder == t->getTid()) {
		depth++;
		return true;
	}

	bool res = Semaphore::tryDown();
	if(res) {
		holder = t->getTid();
		depth++;
	}
	return res;
}

void Mutex::up() {
#if DEBUG_LOCKS
	if(Util::IsPanicStarted())
		return;
#endif

	A_UNUSED Thread *t = Thread::getRunning();
	assert(holder == t->getTid());
	assert(depth > 0);
	if(--depth == 0) {
		holder = INVALID;
		asm volatile ("" : : : "memory");
		Semaphore::up();
	}
}
