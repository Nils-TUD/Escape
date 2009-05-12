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

#include <common.h>
#include <thread.h>
#include <kheap.h>
#include <util.h>
#include <sllist.h>
#include <string.h>
#include <errors.h>

#define THREAD_MAP_SIZE		1024

/* our map for the threads. key is (tid % THREAD_MAP_SIZE) */
static sSLList *threadMap[THREAD_MAP_SIZE] = {NULL};
static sThread *current = NULL;
static tTid nextTid = 0;

void thread_init(sProc *p) {
	/* TODO we don't need a thread_create() since we can create one thread for the first process
	 * once. the process including the thread will be cloned */
	sThread *t = thread_create(p);
	if(t == NULL)
		util_panic("Not enough mem for initial thread");
	current = t;
}

sThread *thread_getRunning(void) {
	return current;
}

sThread *thread_getById(tTid tid) {
	sSLNode *n;
	sThread *t;
	sSLList *list = threadMap[tid % THREAD_MAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		t = (sThread*)n->data;
		if(t->tid == tid)
			return t;
	}
	return NULL;
}

sThread *thread_create(sProc *p) {
	sThread *t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		return NULL;

	/* TODO what state? */
	t->state = ST_RUNNING;
	t->events = EV_NOEVENT;
	t->tid = nextTid++;
	t->process = p;
	/* we'll give the thread a stack later */
	t->stackPages = 0;
	t->ucycleCount = 0;
	t->ucycleStart = 0;
	t->kcycleCount = 0;
	t->kcycleStart = 0;
	t->fpuState = NULL;
	t->signal = 0;
	memcpy(t->name,"main",5);

	/* create list */
	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE];
	if(list == NULL) {
		list = threadMap[t->tid % THREAD_MAP_SIZE] = sll_create();
		if(list == NULL) {
			kheap_free(t);
			return NULL;
		}
	}

	/* append */
	/* TODO WE HAVE TO SEARCH FOR sll_* CALLS THAT CAN FAIL!! */
	if(!sll_append(list,t)) {
		kheap_free(t);
		return NULL;
	}
	return t;
}

s32 thread_clone(void) {
	return 0;
}

s32 thread_destroy(void) {
	return 0;
}
