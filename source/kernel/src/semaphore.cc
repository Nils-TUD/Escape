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
#include <sys/semaphore.h>
#include <sys/spinlock.h>
#include <sys/log.h>
#include <sys/util.h>
#include <assert.h>

#define printEventTrace(...)

void Semaphore::down() {
	SpinLock::acquire(&lock);
	if(--value < 0) {
		Thread *t = Thread::getRunning();
		printEventTrace(Util::getKernelStackTrace(),"[%d] Waiting for %#x ",t->getTid(),this);
		t->wait(EV_MUTEX,(evobj_t)this);
		SpinLock::release(&lock);
		Thread::switchNoSigs();
		SpinLock::acquire(&lock);
	}
	printEventTrace(Util::getKernelStackTrace(),"[%d] L %#x ",Thread::getRunning()->getTid(),this);
	SpinLock::release(&lock);
}

bool Semaphore::tryDown() {
	bool res = false;
	SpinLock::acquire(&lock);
	if(value > 0) {
		printEventTrace(Util::getKernelStackTrace(),"[%d] L %#x ",Thread::getRunning()->getTid(),this);
		value--;
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void Semaphore::up() {
	SpinLock::acquire(&lock);
	if(++value <= 0)
		Sched::wakeup(EV_MUTEX,(evobj_t)this,false);
	printEventTrace(Util::getKernelStackTrace(),"[%d] U %#x %s ",Thread::getRunning()->getTid(),this,
			waiting ? "(Waking up)" : "(No wakeup)");
	SpinLock::release(&lock);
}
