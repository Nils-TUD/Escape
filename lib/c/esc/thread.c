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

#include <esc/common.h>
#include <esc/thread.h>
#include <esc/sllist.h>
#include <stdlib.h>

#define HASHMAP_SIZE	64

typedef struct {
	tTid tid;
	uint key;
	void *val;
} sThreadVal;

/**
 * Assembler-routines
 */
extern int _lock(uint ident,bool global,uint flags);
extern int _unlock(uint ident,bool global);
extern int _waitUnlock(sWaitObject *objects,size_t objCount,uint ident,bool global);

static sSLList *tvalmap[HASHMAP_SIZE];

bool setThreadVal(uint key,void *val) {
	tTid tid = gettid();
	sSLList *list;
	sThreadVal *tval = (sThreadVal*)malloc(sizeof(sThreadVal));
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
	sSLNode *n;
	tTid tid = gettid();
	sSLList *list = tvalmap[tid % HASHMAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		sThreadVal *tval = (sThreadVal*)n->data;
		if(tval->tid == tid && tval->key == key)
			return tval->val;
	}
	return NULL;
}

int wait(uint events,tEvObj object) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return waitm(&obj,1);
}

int lock(uint ident,uint flags) {
	/* nasm doesn't like "lock" as label... */
	return _lock(ident,false,flags);
}

void locku(tULock *l) {
	__asm__ (
		"mov $1,%%ecx;"				/* ecx=1 to lock it for others */
		"lockuLoop:"
		"	xor	%%eax,%%eax;"		/* clear eax */
		"	lock;"					/* lock next instruction */
		"	cmpxchg %%ecx,(%0);"	/* compare l with eax; if equal exchange ecx with l */
		"	jnz		lockuLoop;"		/* try again if not equal */
		: : "D" (l)
	);
}

int lockg(uint ident,uint flags) {
	return _lock(ident,true,flags);
}

int waitUnlock(uint events,tEvObj object,uint ident) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return _waitUnlock(&obj,1,ident,false);
}

int waitUnlockg(uint events,tEvObj object,uint ident) {
	sWaitObject obj;
	obj.events = events;
	obj.object = object;
	return _waitUnlock(&obj,1,ident,true);
}

int unlock(uint ident) {
	return _unlock(ident,false);
}

void unlocku(tULock *l) {
	*l = false;
}

int unlockg(uint ident) {
	return _unlock(ident,true);
}
