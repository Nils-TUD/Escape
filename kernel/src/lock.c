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
#include <proc.h>
#include <kheap.h>
#include <errors.h>
#include <sllist.h>

/* a lock-entry */
typedef struct {
	u8 locked;
	u32 ident;
	u32 waitCount;
} sLock;

/**
 * Searches the lock-entry for the given ident
 *
 * @param ident the ident to search for
 * @return the lock for the given ident or NULL
 */
static sLock *lock_get(u32 ident);

/* our lock-list */
static sSLList *locks = NULL;

/* fortunatly interrupts are disabled in kernel, so the whole stuff here is easy :) */

s32 lock_aquire(tPid pid,u32 ident) {
	sLock *l = lock_get(ident);

	/* if it exists and is locked, wait */
	if(l && l->locked) {
		l->waitCount++;
		do {
			proc_wait(pid,EV_UNLOCK);
			proc_switch();
		}
		while(l->locked);
		/* it is unlocked now, so we can stop waiting and use it */
		l->waitCount--;
	}
	else if(!l) {
		/* create list if not already done */
		if(locks == NULL) {
			locks = sll_create();
			if(locks == NULL)
				return ERR_NOT_ENOUGH_MEM;
		}

		/* create lock */
		l = (sLock*)kheap_alloc(sizeof(sLock));
		if(l == NULL)
			return ERR_NOT_ENOUGH_MEM;
		/* init and append */
		l->ident = ident;
		l->waitCount = 0;
		sll_append(locks,l);
	}

	/* lock it */
	l->locked = true;
	return 0;
}

s32 lock_release(u32 ident) {
	sLock *l = lock_get(ident);
	if(!l)
		return ERR_LOCK_NOT_FOUND;

	/* unlock it */
	l->locked = false;
	proc_wakeupAll(EV_UNLOCK);

	/* if nobody is waiting, we can free the lock-entry */
	if(l->waitCount == 0) {
		sll_removeFirst(locks,l);
		kheap_free(l);
	}
	return 0;
}

static sLock *lock_get(u32 ident) {
	sSLNode *n;
	sLock *l;
	if(locks == NULL)
		return NULL;
	for(n = sll_begin(locks); n != NULL; n = n->next) {
		l = (sLock*)n->data;
		if(l->ident == ident)
			return l;
	}
	return NULL;
}
