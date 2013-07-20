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

#pragma once

#include <sys/common.h>
#include <sys/task/timer.h>

class Thread : public ThreadBase {
	friend class ThreadBase;
public:
	/**
	 * @return the frame mapped at KERNEL_STACK
	 */
	frameno_t getKernelStack() const {
		return kstackFrame;
	}

private:
	frameno_t kstackFrame;
	static Thread *cur;
};

inline size_t ThreadBase::getThreadFrmCnt() {
	return INITIAL_STACK_PAGES;
}

inline Thread *ThreadBase::getRunning() {
	return Thread::cur;
}

inline void ThreadBase::setRunning(Thread *t) {
	Thread::cur = t;
}

inline uint64_t ThreadBase::getTSC() {
	return Timer::getTimestamp();
}

inline uint64_t ThreadBase::ticksPerSec() {
	return 1000;
}

inline uint64_t ThreadBase::getRuntime() const {
	return stats.runtime;
}
