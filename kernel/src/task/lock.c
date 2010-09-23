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
#include <sys/video.h>
#include <esc/sllist.h>
#include <errors.h>

#define LOCK_USED		4

/* a lock-entry */
typedef struct {
	u32 ident;
	u16 flags;
	tPid pid;
	u16 readRefs;
	u16 writeRef;
	u16 waitCount;
} sLock;

/**
 * Searches the lock-entry for the given ident and process-id
 */
static sLock *lock_get(tPid pid,u32 ident,bool free);

static u32 lockCount = 0;
static sLock *locks = NULL;

/* fortunatly interrupts are disabled in kernel, so the whole stuff here is easy :) */

static bool lock_isLocked(sLock *l,u16 flags) {
	if((flags & LOCK_EXCLUSIVE) && (l->readRefs > 0 || l->writeRef))
		return true;
	if(!(flags & LOCK_EXCLUSIVE) && l->writeRef)
		return true;
	return false;
}

s32 lock_aquire(tPid pid,u32 ident,u16 flags) {
	sThread *t = thread_getRunning();
	sLock *l = lock_get(pid,ident,true);
	if(!l)
		return ERR_NOT_ENOUGH_MEM;

	if(l->flags) {
		/* if it exists and is locked, wait */
		u32 event = (flags & LOCK_EXCLUSIVE) ? EV_UNLOCK_EX : EV_UNLOCK_SH;
		while(lock_isLocked(l,flags)) {
			l->waitCount++;
			thread_wait(t->tid,(void*)ident,event);
			thread_switchNoSigs();
			l->waitCount--;
		}
	}
	else {
		l->ident = ident;
		l->pid = pid;
		l->readRefs = 0;
		l->writeRef = 0;
		l->waitCount = 0;
	}

	/* lock it */
	l->flags = flags | LOCK_USED;
	if(flags & LOCK_EXCLUSIVE)
		l->writeRef = 1;
	else
		l->readRefs++;
	return 0;
}

s32 lock_release(tPid pid,u32 ident) {
	sLock *l = lock_get(pid,ident,false);
	if(!l)
		return ERR_LOCK_NOT_FOUND;

	/* unlock */
	if(l->flags & LOCK_EXCLUSIVE) {
		assert(l->writeRef == 1);
		l->writeRef = 0;
	}
	else {
		assert(l->readRefs > 0);
		l->readRefs--;
	}
	/* write-refs can't be > 0 here (either we were the writer -> free now, or we wouldn't
	 * have got the read-lock before) */
	assert(l->writeRef == 0);

	/* are there waiting threads? */
	if(l->waitCount) {
		/* if there are no reads and writes, notify all.
		 * otherwise notify just the threads that wait for a shared lock */
		if(l->readRefs == 0)
			thread_wakeupAll((void*)ident,EV_UNLOCK_EX | EV_UNLOCK_SH);
		else
			thread_wakeupAll((void*)ident,EV_UNLOCK_SH);
	}
	/* if there are no waits and refs anymore and we shouldn't keep it, free the lock */
	else if(l->readRefs == 0 && !(l->flags & LOCK_KEEP))
		l->flags = 0;
	return 0;
}

void lock_releaseAll(tTid tid) {
	/* TODO change to proc; remove all for pid */
}

static sLock *lock_get(tPid pid,u32 ident,bool free) {
	u32 i;
	sLock *fl = NULL;
	for(i = 0; i < lockCount; i++) {
		if(free && !fl && locks[i].flags == 0)
			fl = locks + i;
		else if(locks[i].ident == ident && (locks[i].pid == INVALID_PID || locks[i].pid == pid))
			return locks + i;
	}
	if(!fl && free) {
		sLock *nlocks;
		u32 oldCount = lockCount;
		if(lockCount == 0)
			lockCount = 8;
		else
			lockCount *= 2;
		nlocks = kheap_realloc(locks,lockCount * sizeof(sLock));
		if(nlocks == NULL) {
			lockCount = oldCount;
			return NULL;
		}
		locks = nlocks;
		memset(locks + oldCount,0,(lockCount - oldCount) * sizeof(sLock));
		return locks + oldCount;
	}
	return fl;
}

#if DEBUGGING

void lock_dbg_print(void) {
	u32 i;
	vid_printf("Locks:\n");
	for(i = 0; i < lockCount; i++) {
		sLock *l = locks + i;
		if(l->flags) {
			vid_printf("\t%08x: pid=%u, flags=%#x, reads=%u, writes=%u, waits=%d\n",
					l->ident,l->pid,l->flags,l->readRefs,l->writeRef,l->waitCount);
		}
	}
}

#endif
