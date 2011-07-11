/**
 * $Id: thread.h 965 2011-07-07 10:56:45Z nasmussen $
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

class thread {
	friend std::istream& operator >>(std::istream& is,thread& t);

public:
	typedef pid_t pid_type;
	typedef tid_t tid_type;
	typedef unsigned char state_type;
	typedef size_t size_type;
	typedef unsigned long long cycle_type;

public:
	thread()
		: _tid(0), _pid(0), _procName(std::string()), _state(0), _stackPages(0),
		  _schedCount(0), _syscalls(0), _ucycles(0), _kcycles(0) {
	}
	thread(const thread& t)
		: _tid(t._tid), _pid(t._pid), _procName(t._procName), _state(t._state),
		  _stackPages(t._stackPages), _schedCount(t._schedCount), _syscalls(t._syscalls),
		  _ucycles(t._ucycles), _kcycles(t._kcycles) {
	}
	thread& operator =(const thread& t) {
		clone(t);
		return *this;
	}
	~thread() {
	}

	inline std::string procName() const {
		return _procName;
	};
	inline tid_type tid() const {
		return _tid;
	};
	inline tid_type pid() const {
		return _pid;
	};
	inline state_type state() const {
		return _state;
	};
	inline size_type stackPages() const {
		return _stackPages;
	};
	inline size_type schedCount() const {
		return _schedCount;
	};
	inline size_type syscalls() const {
		return _syscalls;
	};
	inline cycle_type userCycles() const {
		return _ucycles;
	};
	inline cycle_type kernelCycles() const {
		return _kcycles;
	};

private:
	void clone(const thread& t);

private:
	tid_type _tid;
	pid_type _pid;
	std::string _procName;
	state_type _state;
	size_type _stackPages;
	size_type _schedCount;
	size_type _syscalls;
	cycle_type _ucycles;
	cycle_type _kcycles;
};

std::istream& operator >>(std::istream& is,thread& t);

#endif /* THREAD_H_ */
