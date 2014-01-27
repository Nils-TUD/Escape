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

#include <sys/common.h>
#include <sys/task/terminator.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/timer.h>
#include <sys/spinlock.h>
#include <assert.h>

ISList<Thread*> Terminator::deadThreads;
klock_t Terminator::lock;

void Terminator::start() {
	Thread *t = Thread::getRunning();
	SpinLock::acquire(&lock);
	while(1) {
		if(deadThreads.length() == 0) {
			t->wait(EV_TERMINATION,0);
			SpinLock::release(&lock);

			Thread::switchAway();

			SpinLock::acquire(&lock);
		}

		while(deadThreads.length() > 0) {
			Thread *dt = deadThreads.removeFirst();
			/* release the lock while we're killing the thread; the process-module may use us
			 * in this time to add another thread */
			SpinLock::release(&lock);

			while(!dt->beginTerm()) {
				/* ensure that we idle to receive interrupts */
				Timer::sleepFor(t->getTid(),20,true);
				Thread::switchAway();
			}
			Proc::killThread(dt->getTid());

			SpinLock::acquire(&lock);
		}
	}
}

void Terminator::addDead(Thread *t) {
	SpinLock::acquire(&lock);
	/* ensure that we don't add a thread twice */
	if(!(t->getFlags() & T_WILL_DIE)) {
		t->setFlags(t->getFlags() | T_WILL_DIE);
		assert(deadThreads.append(t));
		Sched::wakeup(EV_TERMINATION,0);
	}
	SpinLock::release(&lock);
}
