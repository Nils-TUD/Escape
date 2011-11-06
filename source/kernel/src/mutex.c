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
#include <sys/task/event.h>
#include <sys/mutex.h>
#include <sys/spinlock.h>
#include <sys/log.h>
#include <sys/util.h>
#include <assert.h>

#define util_printEventTrace(...)

static klock_t mutexLock;

void mutex_aquire(sThread *t,mutex_t *m) {
	spinlock_aquire(&mutexLock);
	if(*m & 1) {
		*m += 2;
		util_printEventTrace(util_getKernelStackTrace(),"[%d] Waiting for %#x ",t->tid,m);
		do {
			ev_wait(t,EVI_MUTEX,(evobj_t)m);
			spinlock_release(&mutexLock);
			thread_switchNoSigs();
			spinlock_aquire(&mutexLock);
		}
		while(*m & 1);
		*m -= 2;
	}
	util_printEventTrace(util_getKernelStackTrace(),"[%d] L %#x ",thread_getRunning()->tid,m);
	*m |= 1;
	t->mutexes++;
	spinlock_release(&mutexLock);
}

bool mutex_tryAquire(sThread *t,mutex_t *m) {
	bool res = false;
	spinlock_aquire(&mutexLock);
	if(!(*m & 1)) {
		util_printEventTrace(util_getKernelStackTrace(),"[%d] L %#x ",thread_getRunning()->tid,m);
		*m |= 1;
		t->mutexes++;
		res = true;
	}
	spinlock_release(&mutexLock);
	return res;
}

void mutex_release(sThread *t,mutex_t *m) {
	spinlock_aquire(&mutexLock);
	assert(t->mutexes > 0);
	*m &= ~1;
	t->mutexes--;
	if(*m > 0)
		ev_wakeup(EVI_MUTEX,(evobj_t)m);
	util_printEventTrace(util_getKernelStackTrace(),"[%d] U %#x %s ",thread_getRunning()->tid,m,
			*m > 0 ? "(Waking up)" : "(No wakeup)");
	spinlock_release(&mutexLock);
}
