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
#include <sys/task/event.h>
#include <sys/task/sched.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <sys/log.h>
#include <assert.h>

klock_t Event::lock;
DList<Thread> Event::evlists[EV_COUNT];

bool Event::wait(Thread *t,uint event,evobj_t object) {
	t->event = event;
	t->evobject = object;
	t->block();
	if(event) {
		SpinLock::acquire(&lock);
		evlists[event - 1].append(t);
		SpinLock::release(&lock);
	}
	return true;
}

void Event::wakeup(uint event,evobj_t object) {
	assert(event >= 1 && event <= EV_COUNT);
	DList<Thread> *list = evlists + event - 1;
	SpinLock::acquire(&lock);
	for(auto it = list->begin(); it != list->end(); ) {
		auto old = it++;
		assert(old->event == event);
		if(old->evobject == 0 || old->evobject == object) {
			list->remove(&*old);
			old->unblock();
			old->event = 0;
		}
	}
	SpinLock::release(&lock);
}

bool Event::wakeupThread(Thread *t,uint event) {
	assert(event >= 1 && event <= EV_COUNT);
	bool res = false;
	SpinLock::acquire(&lock);
	if(t->event == event) {
		evlists[event - 1].remove(t);
		t->unblock();
		t->event = 0;
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void Event::removeThread(Thread *t) {
	SpinLock::acquire(&lock);
	if(t->event) {
		evlists[t->event - 1].remove(t);
		t->event = 0;
	}
	SpinLock::release(&lock);
}

void Event::printEvMask(OStream &os,const Thread *t) {
	if(t->event == 0)
		os.writef("-");
	else
		os.writef("%s:%p",getName(t->event),t->evobject);
}

void Event::print(OStream &os) {
	os.writef("Eventlists:\n");
	for(size_t e = 0; e < EV_COUNT; e++) {
		DList<Thread> *list = evlists + e;
		os.writef("\t%s:\n",getName(e + 1));
		for(auto t = list->cbegin(); t != list->cend(); ++t) {
			os.writef("\t\tthread=%d (%d:%s), object=%x",
					t->getTid(),t->getProc()->getPid(),t->getProc()->getProgram(),t->evobject);
			inode_t nodeNo = ((VFSNode*)t->evobject)->getNo();
			if(VFSNode::isValid(nodeNo))
				os.writef("(%s)",((VFSNode*)t->evobject)->getPath());
			os.writef("\n");
		}
	}
}

const char *Event::getName(uint event) {
	static const char *names[] = {
		"CLIENT",
		"RECEIVED_MSG",
		"DATA_READABLE",
		"MUTEX",
		"PIPE_FULL",
		"PIPE_EMPTY",
		"UNLOCK_SH",
		"UNLOCK_EX",
		"REQ_FREE",
		"USER1",
		"USER2",
		"SWAP_JOB",
		"SWAP_WORK",
		"SWAP_FREE",
		"VMM_DONE",
		"THREAD_DIED",
		"CHILD_DIED",
		"TERMINATION",
	};
	return names[event - 1];
}
