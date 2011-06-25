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

#ifndef PROCESS_H_
#define PROCESS_H_

#include <stddef.h>
#include <vector>
#include <string>
#include <limits>

class thread;

class process {
	friend std::istream& operator >>(std::istream& is,process& p);

public:
	typedef pid_t pid_type;
	typedef size_t size_type;
	typedef unsigned long long cycle_type;

private:
	pid_type _pid;
	pid_type _ppid;
	size_type _pages;
	size_type _ownFrames;
	size_type _sharedFrames;
	size_type _swapped;
	mutable size_type _input;
	mutable size_type _output;
	mutable cycle_type _ucycles;
	mutable cycle_type _kcycles;
	std::vector<thread*> _threads;
	std::string _cmd;

public:
	process()
		: _pid(0), _ppid(0), _pages(0), _ownFrames(0), _sharedFrames(0), _swapped(0),
		  _input(0), _output(0), _ucycles(-1), _kcycles(-1), _threads(std::vector<thread*>()),
		  _cmd(std::string()) {
	}
	process(const process& p)
		: _pid(p._pid), _ppid(p._ppid), _pages(p._pages), _ownFrames(p._ownFrames),
		  _sharedFrames(p._sharedFrames), _swapped(p._swapped), _input(p._input),
		  _output(p._output), _ucycles(p._ucycles), _kcycles(p._kcycles), _threads(p._threads),
		  _cmd(p._cmd) {
	}
	process& operator =(const process& p) {
		_pid = p._pid;
		_ppid = p._ppid;
		_pages = p._pages;
		_ownFrames = p._ownFrames;
		_sharedFrames = p._sharedFrames;
		_swapped = p._swapped;
		_ucycles = p._ucycles;
		_kcycles = p._kcycles;
		_threads = p._threads;
		_input = p._input;
		_output = p._output;
		_cmd = p._cmd;
		return *this;
	}
	~process() {
	}

	pid_type pid() const {
		return _pid;
	}
	pid_type ppid() const {
		return _ppid;
	}
	pid_type pages() const {
		return _pages;
	}
	size_type ownFrames() const {
		return _ownFrames;
	}
	size_type sharedFrames() const {
		return _sharedFrames;
	}
	size_type swapped() const {
		return _swapped;
	}
	cycle_type totalCycles() const {
		return userCycles() + kernelCycles();
	}
	cycle_type userCycles() const;
	cycle_type kernelCycles() const;
	size_type input() const {
		return _input;
	}
	size_type output() const {
		return _output;
	}
	const std::vector<thread*>& threads() const {
		return _threads;
	}
	void add_thread(thread* t) {
		_threads.push_back(t);
		// we need to refresh that afterwards
		_ucycles = _kcycles = -1;
	}
	const std::string& command() const {
		return _cmd;
	}
};

std::istream& operator >>(std::istream& is,process& p);

#endif /* PROCESS_H_ */
