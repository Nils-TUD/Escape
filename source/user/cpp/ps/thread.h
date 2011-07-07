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

#ifndef THREAD_H_
#define THREAD_H_

#include <istream>
#include "process.h"

class thread {
	friend std::istream& operator >>(std::istream& is,thread& t);

public:
	typedef process::pid_type pid_type;
	typedef tid_t tid_type;
	typedef unsigned char state_type;
	typedef process::size_type size_type;
	typedef process::cycle_type cycle_type;

private:
	tid_type _tid;
	pid_type _pid;
	state_type _state;
	size_type _stackPages;
	size_type _schedCount;
	size_type _syscalls;
	cycle_type _ucycles;
	cycle_type _kcycles;

public:
	thread()
		: _tid(0), _pid(0), _state(0), _stackPages(0), _schedCount(0), _syscalls(0),
		  _ucycles(0), _kcycles(0) {
	}
	thread(const thread& t)
		: _tid(t._tid), _pid(t._pid), _state(t._state), _stackPages(t._stackPages),
		  _schedCount(t._schedCount), _syscalls(t._syscalls),
		  _ucycles(t._ucycles), _kcycles(t._kcycles) {
	}
	thread& operator =(const thread& t) {
		clone(t);
		return *this;
	}
	~thread() {
	}

	tid_type tid() const {
		return _tid;
	}
	tid_type pid() const {
		return _pid;
	}
	state_type state() const {
		return _state;
	}
	size_type stackPages() const {
		return _stackPages;
	}
	size_type schedCount() const {
		return _schedCount;
	}
	size_type syscalls() const {
		return _syscalls;
	}
	cycle_type userCycles() const {
		return _ucycles;
	}
	cycle_type kernelCycles() const {
		return _kcycles;
	}

private:
	void clone(const thread& t) {
		_tid = t._tid;
		_pid = t._pid;
		_state = t._state;
		_stackPages = t._stackPages;
		_schedCount = t._schedCount;
		_syscalls = t._syscalls;
		_ucycles = t._ucycles;
		_kcycles = t._kcycles;
	}
};

std::istream& operator >>(std::istream& is,thread& t);

#endif /* THREAD_H_ */
