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

#include <sys/common.h>
#include <sys/task/terminator.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/timer.h>
#include <sys/mem/sllnodes.h>
#include <sys/spinlock.h>
#include <esc/sllist.h>
#include <assert.h>

sSLList Terminator::deadThreads;
klock_t Terminator::termLock;

void Terminator::init() {
	sll_init(&deadThreads,slln_allocNode,slln_freeNode);
}

void Terminator::start() {
	Thread *t = Thread::getRunning();
	spinlock_aquire(&termLock);
	while(1) {
		if(sll_length(&deadThreads) == 0) {
			Event::wait(t,EVI_TERMINATION,0);
			spinlock_release(&termLock);

			Thread::switchAway();

			spinlock_aquire(&termLock);
		}

		while(sll_length(&deadThreads) > 0) {
			Thread *dt = (Thread*)sll_removeFirst(&deadThreads);
			/* release the lock while we're killing the thread; the process-module may use us
			 * in this time to add another thread */
			spinlock_release(&termLock);

			while(!dt->beginTerm()) {
				/* ensure that we idle to receive interrupts */
				Timer::sleepFor(t->getTid(),20,true);
				Thread::switchAway();
			}
			Proc::killThread(dt->getTid());

			spinlock_aquire(&termLock);
		}
	}
}

void Terminator::addDead(Thread *t) {
	spinlock_aquire(&termLock);
	/* ensure that we don't add a thread twice */
	if(!(t->getFlags() & T_WILL_DIE)) {
		t->setFlags(t->getFlags() | T_WILL_DIE);
		assert(sll_append(&deadThreads,t));
		Event::wakeup(EVI_TERMINATION,0);
	}
	spinlock_release(&termLock);
}
