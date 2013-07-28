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
#include <sys/mem/sllnodes.h>
#include <sys/task/event.h>
#include <sys/task/sched.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <esc/sllist.h>
#include <sys/spinlock.h>
#include <sys/video.h>
#include <assert.h>

klock_t Event::lock;
sWait Event::waits[MAX_WAIT_COUNT];
sWait *Event::waitFree;
Event::WaitList Event::evlists[EV_COUNT];

void Event::init(void) {
	size_t i;
	for(i = 0; i < EV_COUNT; i++) {
		evlists[i].begin = NULL;
		evlists[i].last = NULL;
	}

	waitFree = waits;
	waitFree->next = NULL;
	for(i = 1; i < MAX_WAIT_COUNT; i++) {
		waits[i].next = waitFree;
		waitFree = waits + i;
	}
}

bool Event::wait(Thread *t,size_t evi,evobj_t object) {
	bool res = false;
	sWait *w;
	SpinLock::acquire(&lock);
	w = t->waits;
	while(w && w->tnext)
		w = w->tnext;
	if(doWait(t,evi,object,&t->waits,w) != NULL) {
		t->block();
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

bool Event::waitObjects(Thread *t,const WaitObject *objects,size_t objCount) {
	size_t i,e;
	sWait *w;
	SpinLock::acquire(&lock);
	w = t->waits;
	while(w && w->tnext)
		w = w->tnext;

	for(i = 0; i < objCount; i++) {
		uint events = objects[i].events;
		if(events != 0) {
			for(e = 0; events && e < EV_COUNT; e++) {
				if(events & (1 << e)) {
					w = doWait(t,e,objects[i].object,&t->waits,w);
					if(w == NULL) {
						doRemoveThread(t);
						SpinLock::release(&lock);
						return false;
					}
					events &= ~(1 << e);
				}
			}
		}
	}
	t->block();
	SpinLock::release(&lock);
	return true;
}

void Event::wakeup(size_t evi,evobj_t object) {
	tid_t tids[MAX_WAKEUPS];
	WaitList *list = evlists + evi;
	sWait *w;
	size_t i = 0;
	SpinLock::acquire(&lock);
	w = list->begin;
	while(w != NULL) {
		if(w->object == 0 || w->object == object) {
			if(i < MAX_WAKEUPS && (i == 0 || tids[i - 1] != w->tid))
				tids[i++] = w->tid;
			else {
				/* all slots in use, so remove this threads and start from the beginning to find
				 * more. hopefully, this will happen nearly never :) */
				for(; i > 0; i--) {
					Thread *t = Thread::getById(tids[i - 1]);
					doRemoveThread(t);
					t->unblock();
				}
				w = list->begin;
				continue;
			}
		}
		w = w->next;
	}
	for(; i > 0; i--) {
		Thread *t = Thread::getById(tids[i - 1]);
		doRemoveThread(t);
		t->unblock();
	}
	SpinLock::release(&lock);
}

void Event::wakeupm(uint events,evobj_t object) {
	size_t e;
	for(e = 0; events && e < EV_COUNT; e++) {
		if(events & (1 << e)) {
			wakeup(e,object);
			events &= ~(1 << e);
		}
	}
}

bool Event::wakeupThread(Thread *t,uint events) {
	bool res = false;
	SpinLock::acquire(&lock);
	if(t->events & events) {
		doRemoveThread(t);
		t->unblock();
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void Event::printEvMask(OStream &os,const Thread *t) {
	uint e;
	if(t->events == 0)
		os.writef("-");
	else {
		for(e = 0; e < EV_COUNT; e++) {
			if(t->events & (1 << e))
				os.writef("%s ",getName(e));
		}
	}
}

void Event::print(OStream &os) {
	size_t e;
	os.writef("Eventlists:\n");
	for(e = 0; e < EV_COUNT; e++) {
		WaitList *list = evlists + e;
		sWait *w = list->begin;
		os.writef("\t%s:\n",getName(e));
		while(w != NULL) {
			inode_t nodeNo;
			Thread *t = Thread::getById(w->tid);
			os.writef("\t\tthread=%d (%d:%s), object=%x",
					t->getTid(),t->getProc()->getPid(),t->getProc()->getCommand(),w->object);
			nodeNo = vfs_node_getNo((sVFSNode*)w->object);
			if(vfs_node_isValid(nodeNo))
				os.writef("(%s)",vfs_node_getPath(nodeNo));
			os.writef("\n");
			w = w->next;
		}
	}
}

void Event::doRemoveThread(Thread *t) {
	if(t->events) {
		sWait *w = t->waits;
		while(w != NULL) {
			sWait *nw = w->tnext;
			if(w->prev)
				w->prev->next = w->next;
			else
				evlists[w->evi].begin = w->next;
			if(w->next)
				w->next->prev = w->prev;
			else
				evlists[w->evi].last = w->prev;
			freeWait(w);
			w = nw;
		}
		t->waits = NULL;
		t->events = 0;
	}
}

sWait *Event::doWait(Thread *t,size_t evi,evobj_t object,sWait **begin,sWait *prev) {
	WaitList *list = evlists + evi;
	sWait *w = allocWait();
	if(!w)
		return NULL;
	w->tid = t->getTid();
	w->evi = evi;
	w->object = object;
	/* insert into list */
	w->next = NULL;
	w->prev = list->last;
	if(list->last)
		list->last->next = w;
	else
		list->begin = w;
	list->last = w;
	/* add to thread */
	t->events |= 1 << evi;
	if(prev)
		prev->tnext = w;
	else
		*begin = w;
	w->tnext = NULL;
	return w;
}

sWait *Event::allocWait(void) {
	sWait *res = waitFree;
	if(res == NULL)
		return NULL;
	waitFree = waitFree->next;
	return res;
}

void Event::freeWait(sWait *w) {
	w->next = waitFree;
	waitFree = w;
}

const char *Event::getName(size_t evi) {
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
	return names[evi];
}
