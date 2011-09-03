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
#include <sys/mem/sllnodes.h>
#include <sys/klock.h>
#include <esc/sllist.h>
#include <assert.h>

static sSLList deadThreads;
static klock_t termLock;

void term_init(void) {
	sll_init(&deadThreads,slln_allocNode,slln_freeNode);
}

void term_start(void) {
	sThread *t = thread_getRunning();
	klock_aquire(&termLock);
	while(1) {
		if(sll_length(&deadThreads) == 0) {
			ev_wait(t,EVI_TERMINATION,0);
			klock_release(&termLock);

			thread_switch();

			klock_aquire(&termLock);
		}

		while(sll_length(&deadThreads) > 0) {
			sThread *dt = (sThread*)sll_removeFirst(&deadThreads);
			/* release the lock while we're killing the thread; the process-module may use us
			 * in this time to add another thread */
			klock_release(&termLock);
			while(thread_isRunning(dt))
				thread_switch();
			proc_killThread(dt->tid);
			klock_aquire(&termLock);
		}
	}
}

void term_addDead(sThread *t) {
	klock_aquire(&termLock);
	assert(sll_append(&deadThreads,t));
	ev_wakeup(EVI_TERMINATION,0);
	klock_release(&termLock);
}
