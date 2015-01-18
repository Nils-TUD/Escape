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

#include <task/proc.h>
#include <task/terminator.h>
#include <task/thread.h>
#include <task/timer.h>
#include <assert.h>
#include <common.h>
#include <spinlock.h>

esc::ISList<Thread*> Terminator::deadThreads;
SpinLock Terminator::lock;
BaseSem Terminator::sem(0);

void Terminator::start() {
	lock.down();
	while(1) {
		sem.down(&lock);

		Thread *dt = deadThreads.removeFirst();

		/* better do that unlocked; we might block on a mutex */
		lock.up();
		while(dt->getState() != Thread::ZOMBIE)
			Thread::switchAway();
		Proc::killThread(dt);
		lock.down();
	}
}

void Terminator::addDead(Thread *t) {
	LockGuard<SpinLock> g(&lock);
	sassert(deadThreads.append(t));
	sem.up();
}
