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

#define printEventTrace(...)

static klock_t mutexLock;

void mutex_acquire(mutex_t *m) {
	Thread *t = Thread::getRunning();
	SpinLock::acquire(&mutexLock);
	if(*m & 1) {
		*m += 2;
		printEventTrace(Util::getKernelStackTrace(),"[%d] Waiting for %#x ",t->getTid(),m);
		do {
			Event::wait(t,EVI_MUTEX,(evobj_t)m);
			SpinLock::release(&mutexLock);
			Thread::switchNoSigs();
			SpinLock::acquire(&mutexLock);
		}
		while(*m & 1);
		*m -= 2;
	}
	printEventTrace(Util::getKernelStackTrace(),"[%d] L %#x ",Thread::getRunning()->getTid(),m);
	*m |= 1;
	t->addResource();
	SpinLock::release(&mutexLock);
}

bool mutex_tryAquire(mutex_t *m) {
	Thread *t = Thread::getRunning();
	bool res = false;
	SpinLock::acquire(&mutexLock);
	if(!(*m & 1)) {
		printEventTrace(Util::getKernelStackTrace(),"[%d] L %#x ",Thread::getRunning()->getTid(),m);
		*m |= 1;
		t->addResource();
		res = true;
	}
	SpinLock::release(&mutexLock);
	return res;
}

void mutex_release(mutex_t *m) {
	Thread *t = Thread::getRunning();
	SpinLock::acquire(&mutexLock);
	*m &= ~1;
	t->remResource();
	if(*m > 0)
		Event::wakeup(EVI_MUTEX,(evobj_t)m);
	printEventTrace(Util::getKernelStackTrace(),"[%d] U %#x %s ",Thread::getRunning()->getTid(),m,
			*m > 0 ? "(Waking up)" : "(No wakeup)");
	SpinLock::release(&mutexLock);
}
