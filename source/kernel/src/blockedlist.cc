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
#include <assert.h>
#include <blockedlist.h>
#include <common.h>

void BlockedList::block(Thread *t) {
	assert(t == Thread::getRunning());
	list.append(t);
	Sched::block(t);
}

void BlockedList::remove(Thread *t) {
	list.remove(t);
}

void BlockedList::wakeup() {
	Thread *t = list.removeFirst();
	if(t)
		Sched::unblock(t);
}

void BlockedList::wakeupAll() {
	for(auto it = list.begin(); it != list.end(); ++it)
		Sched::unblock(&*it);
	list.clear();
}
