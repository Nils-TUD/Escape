/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/sllist.h>
#include <sys/video.h>
#include <assert.h>

#define MAX_WAIT_COUNT		1024

typedef struct sWait {
	tTid tid;
	tEvObj object;
	struct sWait *next;
} sWait;

static sWait *ev_allocWait(void);
static void ev_freeWait(sWait *w);
static const char *ev_getName(size_t evi);

static sWait waits[MAX_WAIT_COUNT];
static sWait *waitFree;
static sSLList evlists[EV_COUNT];

void ev_init(void) {
	size_t i;
	for(i = 0; i < EV_COUNT; i++)
		sll_init(evlists + i,slln_allocNode,slln_freeNode);

	waitFree = waits;
	waitFree->next = NULL;
	for(i = 1; i < MAX_WAIT_COUNT; i++) {
		waits[i].next = waitFree;
		waitFree = waits + i;
	}
}

bool ev_waitsFor(tTid tid,uint events) {
	sThread *t = thread_getById(tid);
	return t->events & events;
}

bool ev_wait(tTid tid,size_t evi,tEvObj object) {
	sThread *t = thread_getById(tid);
	sSLList *list = evlists + evi;
	sWait *w = ev_allocWait();
	if(!w)
		return false;
	w->tid = tid;
	w->object = object;
	if(!sll_append(list,w)) {
		ev_freeWait(w);
		return false;
	}
	t->events |= 1 << evi;
	sched_setBlocked(t);
	return true;
}

bool ev_waitObjects(tTid tid,const sWaitObject *objects,size_t objCount) {
	size_t i,e;
	for(i = 0; i < objCount; i++) {
		uint events = objects[i].events;
		if(events == 0)
			sched_setBlocked(thread_getById(tid));
		else {
			for(e = 0; events && e < EV_COUNT; e++) {
				if(events & (1 << e)) {
					if(!ev_wait(tid,e,objects[i].object)) {
						ev_removeThread(tid);
						return false;
					}
					events &= ~(1 << e);
				}
			}
		}
	}
	return true;
}

void ev_wakeup(size_t evi,tEvObj object) {
	sSLList *list = evlists + evi;
	sSLNode *n;
	for(n = sll_begin(list); n != NULL; ) {
		sWait *w = (sWait*)n->data;
		if(w->object == 0 || w->object == object) {
			ev_removeThread(w->tid);
			/* we have to start at the beginning again, since its possible that the thread
			 * is before this node for a different object */
			n = sll_begin(list);
		}
		else
			n = n->next;
	}
}

void ev_wakeupm(uint events,tEvObj object) {
	size_t e;
	for(e = 0; events && e < EV_COUNT; e++) {
		if(events & (1 << e)) {
			ev_wakeup(e,object);
			events &= ~(1 << e);
		}
	}
}

bool ev_wakeupThread(tTid tid,uint events) {
	sThread *t = thread_getById(tid);
	if(t->events & events) {
		ev_removeThread(tid);
		return true;
	}
	return false;
}

void ev_removeThread(tTid tid) {
	sThread *t = thread_getById(tid);
	if(t->events) {
		size_t e;
		for(e = 0; t->events && e < EV_COUNT; e++) {
			if(t->events & (1 << e)) {
				sSLNode *n,*tn,*p = NULL;
				sSLList *list = evlists + e;
				for(n = sll_begin(list); n != NULL; ) {
					sWait *w = (sWait*)n->data;
					if(w->tid == tid) {
						tn = n->next;
						sll_removeNode(list,n,p);
						ev_freeWait(w);
						n = tn;
					}
					else {
						p = n;
						n = n->next;
					}
				}
				t->events &= ~(1 << e);
			}
		}
		sched_setReady(t);
		assert(t->events == 0);
	}
}

void ev_printEvMask(uint mask) {
	uint e;
	for(e = 0; e < EV_COUNT; e++) {
		if(mask & (1 << e))
			vid_printf("%s ",ev_getName(e));
	}
}

void ev_print(void) {
	size_t e;
	vid_printf("Eventlists:\n");
	for(e = 0; e < EV_COUNT; e++) {
		sSLList *list = evlists + e;
		sSLNode *n;
		vid_printf("\t%s:\n",ev_getName(e));
		for(n = sll_begin(list); n != NULL; n = n->next) {
			sWait *w = (sWait*)n->data;
			sThread *t = thread_getById(w->tid);
			vid_printf("\t\tthread=%d (%d:%s), object=%x",
					t->tid,t->proc->pid,t->proc->command,w->object);
			tInodeNo nodeNo = vfs_node_getNo((sVFSNode*)w->object);
			if(vfs_node_isValid(nodeNo))
				vid_printf("(%s)",vfs_node_getPath(nodeNo));
			vid_printf("\n");
		}
	}
}

static sWait *ev_allocWait(void) {
	sWait *res = waitFree;
	if(res == NULL)
		return NULL;
	waitFree = waitFree->next;
	return res;
}

static void ev_freeWait(sWait *w) {
	w->next = waitFree;
	waitFree = w;
}

static const char *ev_getName(size_t evi) {
	static const char *names[] = {
		"CLIENT",
		"RECEIVED_MSG",
		"CHILD_DIED",
		"DATA_READABLE",
		"UNLOCK_SH",
		"PIPE_FULL",
		"PIPE_EMPTY",
		"VM86_READY",
		"REQ_REPLY",
		"SWAP_DONE",
		"SWAP_WORK",
		"SWAP_FREE",
		"VMM_DONE",
		"THREAD_DIED",
		"USER1",
		"USER2",
		"REQ_FREE",
		"UNLOCK_EX",
	};
	return names[evi];
}
