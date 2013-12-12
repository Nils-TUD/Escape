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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/sllist.h>
#include <stdio.h>
#include <stdlib.h>

#define HASHMAP_SIZE	64
#define SEM_LOCAL_PATH	"/system/processes/self/sems/"

typedef struct {
	tid_t tid;
	uint key;
	void *val;
} sThreadVal;

static sThreadVal *getThreadValEntry(uint key);

static sSLList *tvalmap[HASHMAP_SIZE];

bool setThreadVal(uint key,void *val) {
	tid_t tid = gettid();
	sSLList *list;
	sThreadVal *tval = getThreadValEntry(key);
	if(tval) {
		tval->val = val;
		return true;
	}

	tval = (sThreadVal*)malloc(sizeof(sThreadVal));
	if(!tval)
		return false;

	list = tvalmap[tid % HASHMAP_SIZE];
	if(!list) {
		list = tvalmap[tid % HASHMAP_SIZE] = sll_create();
		if(!list) {
			free(tval);
			return false;
		}
	}

	tval->tid = tid;
	tval->key = key;
	tval->val = val;
	if(!sll_append(list,tval)) {
		free(tval);
		return false;
	}
	return true;
}

void *getThreadVal(uint key) {
	sThreadVal *entry = getThreadValEntry(key);
	if(!entry)
		return NULL;
	return entry->val;
}

static sThreadVal *getThreadValEntry(uint key) {
	sSLNode *n;
	tid_t tid = gettid();
	sSLList *list = tvalmap[tid % HASHMAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		sThreadVal *tval = (sThreadVal*)n->data;
		if(tval->tid == tid && tval->key == key)
			return tval;
	}
	return NULL;
}

int gsemopen(const char *name) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/sems/%s",name);
	return open(path,IO_SEM | IO_CREATE);
}

int gsemjoin(const char *name) {
	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/system/sems/%s",name);
	return open(path,IO_SEM);
}

int waitunlock(uint event,evobj_t object,ulong ident) {
	return syscall4(SYSCALL_WAITUNLOCK,event,object,ident,false);
}

int waitunlockg(uint event,evobj_t object,ulong ident) {
	return syscall4(SYSCALL_WAITUNLOCK,event,object,ident,true);
}
