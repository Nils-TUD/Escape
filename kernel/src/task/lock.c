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
#include <sys/task/lock.h>
#include <sys/task/thread.h>
#include <sys/mem/kheap.h>
#include <errors.h>
#include <esc/sllist.h>

#define LOCK_MAP_SIZE	128

/* a lock-entry */
typedef struct {
	u8 locked;
	u32 ident;
	tTid owner;
	tPid pid;
	u32 waitCount;
} sLock;


/**
 * Releases the given lock
 */
static void lock_relLock(sLock *l);
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
			thread_wait(tid,(void*)ident,EV_UNLOCK);
			thread_switchNoSigs();
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
	l->owner = tid;
	l->locked = true;
	return 0;
}

s32 lock_release(tPid pid,u32 ident) {
	sLock *l = lock_get(pid,ident);
	if(!l)
		return ERR_LOCK_NOT_FOUND;

	lock_relLock(l);
	return 0;
}

void lock_releaseAll(tTid tid) {
	sSLNode *n;
	sSLList **list = locks;
	sLock *l;
	u32 i;
	for(i = 0; i < LOCK_MAP_SIZE; i++) {
		if(list[i]) {
			for(n = sll_begin(list[i]); n != NULL; ) {
				l = (sLock*)n->data;
				/* store next here because lock_relLock() may delete this node */
				n = n->next;
				if(l->owner == tid)
					lock_relLock(l);
			}
		}
	}
}

static void lock_relLock(sLock *l) {
	/* unlock it */
	l->locked = false;
	l->owner = INVALID_TID;
	thread_wakeupAll((void*)l->ident,EV_UNLOCK);

	/* if nobody is waiting, we can free the lock-entry */
	if(l->waitCount == 0) {
		sll_removeFirst(locks[l->ident % LOCK_MAP_SIZE],l);
		kheap_free(l);
	}
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
