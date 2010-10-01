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
#include <sys/task/event.h>
#include <sys/mem/kheap.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <string.h>
#include <errors.h>

#define LOCK_USED		4

/* a lock-entry */
typedef struct {
	u32 ident;
	u16 flags;
	tPid pid;
	volatile u16 readRefs;
	volatile u16 writeRef;
	u16 waitCount;
} sLock;

/**
 * Searches the lock-entry for the given ident and process-id
 */
static s32 lock_get(tPid pid,u32 ident,bool free);

static s32 lockCount = 0;
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
	s32 i = lock_get(pid,ident,true);
	sLock *l;
	if(i < 0)
		return ERR_NOT_ENOUGH_MEM;

	/* note that we have to use the index here since locks can chance if another threads reallocates
	 * it in the meanwhile */
	l = locks + i;
	if(l->flags) {
		/* if it exists and is locked, wait */
		u32 event = (flags & LOCK_EXCLUSIVE) ? EVI_UNLOCK_EX : EVI_UNLOCK_SH;
		while(lock_isLocked(locks + i,flags)) {
			locks[i].waitCount++;
			ev_wait(t->tid,event,(void*)ident);
			thread_switchNoSigs();
			locks[i].waitCount--;
		}
		l = locks + i;
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
	s32 i = lock_get(pid,ident,false);
	sLock *l = locks + i;
	if(i < 0)
		return ERR_LOCK_NOT_FOUND;

	/* unlock */
	if(l->flags & LOCK_EXCLUSIVE) {
		vassert(l->writeRef == 1,"ident=%#08x, pid=%d",l->ident,l->pid);
		l->writeRef = 0;
	}
	else {
		vassert(l->readRefs > 0,"ident=%#08x, pid=%d",l->ident,l->pid);
		l->readRefs--;
	}
	/* write-refs can't be > 0 here (either we were the writer -> free now, or we wouldn't
	 * have got the read-lock before) */
	assert(l->writeRef == 0);

	/* are there waiting threads? */
	if(l->waitCount) {
		/* if there are no reads and writes, notify all.
		 * otherwise notify just the threads that wait for a shared lock */
		ev_wakeup(EVI_UNLOCK_SH,(void*)ident);
		if(l->readRefs == 0)
			ev_wakeup(EVI_UNLOCK_EX,(void*)ident);
	}
	/* if there are no waits and refs anymore and we shouldn't keep it, free the lock */
	else if(l->readRefs == 0 && !(l->flags & LOCK_KEEP))
		l->flags = 0;
	return 0;
}

void lock_releaseAll(tPid pid) {
	s32 i;
	for(i = 0; i < lockCount; i++) {
		if(locks[i].flags && locks[i].pid == pid)
			locks[i].flags = 0;
	}
}

static s32 lock_get(tPid pid,u32 ident,bool free) {
	s32 i,freeIdx = -1;
	for(i = 0; i < lockCount; i++) {
		if(free && freeIdx == -1 && locks[i].flags == 0)
			freeIdx = i;
		else if(locks[i].flags && locks[i].ident == ident &&
				(locks[i].pid == INVALID_PID || locks[i].pid == pid))
			return i;
	}
	if(freeIdx == -1 && free) {
		sLock *nlocks;
		s32 oldCount = lockCount;
		if(lockCount == 0)
			lockCount = 8;
		else
			lockCount *= 2;
		nlocks = kheap_realloc(locks,lockCount * sizeof(sLock));
		if(nlocks == NULL) {
			lockCount = oldCount;
			return ERR_NOT_ENOUGH_MEM;
		}
		locks = nlocks;
		memset(locks + oldCount,0,(lockCount - oldCount) * sizeof(sLock));
		return oldCount;
	}
	return freeIdx;
}

#if DEBUGGING

void lock_dbg_print(void) {
	s32 i;
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
