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
	typedef process::cycle_type cycle_type;

private:
	cycle_type _ucycles;
	cycle_type _kcycles;

public:
	thread()
		: _ucycles(0), _kcycles(0) {
	}
	thread(const thread& t)
		: _ucycles(t._ucycles), _kcycles(t._kcycles) {
	}
	thread& operator =(const thread& t) {
		_ucycles = t._ucycles;
		_kcycles = t._kcycles;
		return *this;
	}
	~thread() {
	}

	cycle_type userCycles() const {
		return _ucycles;
	}
	cycle_type kernelCycles() const {
		return _kcycles;
	}
};

std::istream& operator >>(std::istream& is,thread& t);

#endif /* THREAD_H_ */
