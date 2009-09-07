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
#include <lock.h>
#include <thread.h>
#include <kheap.h>
#include <errors.h>
#include <sllist.h>

#define LOCK_MAP_SIZE	128

/* a lock-entry */
typedef struct {
	u8 locked;
	u32 ident;
	tPid pid;
	u32 waitCount;
} sLock;

/**
 * Searches the lock-entry for the given ident and process-id
 *
 * @param pid the process-id
 * @param ident the ident to search for
 * @return the lock for the given ident or NULL
 */
static sLock *lock_get(tPid pid,u32 ident);

/* our lock-map; key = ident % LOCK_MAP_SIZE */
static sSLList *locks[LOCK_MAP_SIZE] = {NULL};

/* fortunatly interrupts are disabled in kernel, so the whole stuff here is easy :) */

s32 lock_aquire(tTid tid,tPid pid,u32 ident) {
	sLock *l = lock_get(pid,ident);

	/* if it exists and is locked, wait */
	if(l && l->locked) {
		volatile sLock *myl = l;
		myl->waitCount++;
		do {
			thread_wait(tid,EV_UNLOCK);
			thread_switchInKernel();
		}
		while(myl->locked);
		/* it is unlocked now, so we can stop waiting and use it */
		myl->waitCount--;
	}
	else if(!l) {
		sSLList *list = locks[ident % LOCK_MAP_SIZE];
		/* create list if not already done */
		if(list == NULL) {
			list = locks[ident % LOCK_MAP_SIZE] = sll_create();
			if(list == NULL)
				return ERR_NOT_ENOUGH_MEM;
		}

		/* create lock */
		l = (sLock*)kheap_alloc(sizeof(sLock));
		if(l == NULL)
			return ERR_NOT_ENOUGH_MEM;
		/* init and append */
		l->pid = pid;
		l->ident = ident;
		l->waitCount = 0;
		if(!sll_append(list,l)) {
			kheap_free(l);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* lock it */
	l->locked = true;
	return 0;
}

s32 lock_release(tPid pid,u32 ident) {
	sLock *l = lock_get(pid,ident);
	if(!l)
		return ERR_LOCK_NOT_FOUND;

	/* unlock it */
	l->locked = false;
	thread_wakeupAll(EV_UNLOCK);

	/* if nobody is waiting, we can free the lock-entry */
	if(l->waitCount == 0) {
		sll_removeFirst(locks[ident % LOCK_MAP_SIZE],l);
		kheap_free(l);
	}
	return 0;
}

static sLock *lock_get(tPid pid,u32 ident) {
	sSLNode *n;
	sSLList *list;
	sLock *l;
	list = locks[ident % LOCK_MAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		l = (sLock*)n->data;
		if((l->pid == INVALID_PID || l->pid == pid) && l->ident == ident)
			return l;
	}
	return NULL;
}
