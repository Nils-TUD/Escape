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
#include <sys/task/lock.h>
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

size_t Lock::lockCount = 0;
Lock::Entry *Lock::locks = NULL;
klock_t Lock::klock;

bool Lock::isLocked(const Entry *l,ushort flags) {
	bool res = false;
	if((flags & EXCLUSIVE) && (l->readRefs > 0 || l->writer != INVALID_TID))
		res = true;
	else if(!(flags & EXCLUSIVE) && l->writer != INVALID_TID)
		res = true;
	return res;
}

int Lock::acquire(pid_t pid,ulong ident,ushort flags) {
	Thread *t = Thread::getRunning();
	SpinLock::acquire(&klock);
	ssize_t i = get(pid,ident,true);
	if(i < 0) {
		SpinLock::release(&klock);
		return -ENOMEM;
	}

	/* note that we have to use the index here since locks can change if another threads reallocates
	 * it in the meanwhile */
	Entry *l = locks + i;
	if(l->flags) {
		/* if it exists and is locked, wait */
		uint event = (flags & EXCLUSIVE) ? EV_UNLOCK_EX : EV_UNLOCK_SH;
		/* TODO don't panic here, just return and continue using the lock */
		assert(l->writer != t->getTid());
		while(isLocked(locks + i,flags)) {
			locks[i].waitCount++;
			t->wait(event,(evobj_t)ident);
			SpinLock::release(&klock);

			Thread::switchNoSigs();

			SpinLock::acquire(&klock);
			locks[i].waitCount--;
		}
		l = locks + i;
	}
	else {
		l->ident = ident;
		l->pid = pid;
		l->readRefs = 0;
		l->writer = INVALID_TID;
		l->waitCount = 0;
	}

	/* lock it */
	l->flags = flags | USED;
	if(flags & EXCLUSIVE)
		l->writer = t->getTid();
	else
		l->readRefs++;
	SpinLock::release(&klock);
	return 0;
}

int Lock::release(pid_t pid,ulong ident) {
	SpinLock::acquire(&klock);
	ssize_t i = get(pid,ident,false);
	Entry *l = locks + i;
	if(i < 0) {
		SpinLock::release(&klock);
		return -ENOENT;
	}

	/* unlock */
	if(l->flags & EXCLUSIVE) {
		vassert(l->writer != INVALID_TID,"ident=%#08x, pid=%d",l->ident,l->pid);
		l->writer = INVALID_TID;
	}
	else {
		vassert(l->readRefs > 0,"ident=%#08x, pid=%d",l->ident,l->pid);
		l->readRefs--;
	}
	/* writer can't be != INVALID_TID here (either we were the writer -> free now, or we wouldn't
	 * have got the read-lock before) */
	assert(l->writer == INVALID_TID);

	/* are there waiting threads? */
	if(l->waitCount) {
		/* if there are no reads and writes, notify all.
		 * otherwise notify just the threads that wait for a shared lock */
		Sched::wakeup(EV_UNLOCK_SH,(evobj_t)ident);
		if(l->readRefs == 0)
			Sched::wakeup(EV_UNLOCK_EX,(evobj_t)ident);
	}
	/* if there are no waits and refs anymore and we shouldn't keep it, free the lock */
	else if(l->readRefs == 0 && !(l->flags & KEEP))
		l->flags = 0;
	SpinLock::release(&klock);
	return 0;
}

void Lock::releaseAll(pid_t pid) {
	SpinLock::acquire(&klock);
	for(size_t i = 0; i < lockCount; i++) {
		if(locks[i].flags && locks[i].pid == pid)
			locks[i].flags = 0;
	}
	SpinLock::release(&klock);
}

void Lock::print(OStream &os) {
	os.writef("Locks:\n");
	for(size_t i = 0; i < lockCount; i++) {
		Entry *l = locks + i;
		if(l->flags) {
			os.writef("\t%08lx: pid=%u, flags=%#x, reads=%u, writer=%d, waits=%d\n",
					l->ident,l->pid,l->flags,l->readRefs,l->writer,l->waitCount);
		}
	}
}

ssize_t Lock::get(pid_t pid,ulong ident,bool free) {
	ssize_t freeIdx = -1;
	for(size_t i = 0; i < lockCount; i++) {
		if(free && freeIdx == -1 && locks[i].flags == 0)
			freeIdx = i;
		else if(locks[i].flags && locks[i].ident == ident &&
				(locks[i].pid == INVALID_PID || locks[i].pid == pid))
			return i;
	}
	if(freeIdx == -1 && free) {
		size_t oldCount = lockCount;
		if(lockCount == 0)
			lockCount = 8;
		else
			lockCount *= 2;
		Entry *nlocks = (Entry*)Cache::realloc(locks,lockCount * sizeof(Entry));
		if(nlocks == NULL) {
			lockCount = oldCount;
			return -ENOMEM;
		}
		locks = nlocks;
		memclear(locks + oldCount,(lockCount - oldCount) * sizeof(Entry));
		return oldCount;
	}
	return freeIdx;
}
