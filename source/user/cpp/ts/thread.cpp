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

#include "thread.h"

void thread::clone(const thread& t) {
	_tid = t._tid;
	_pid = t._pid;
	_procName = t._procName;
	_state = t._state;
	_stackPages = t._stackPages;
	_schedCount = t._schedCount;
	_syscalls = t._syscalls;
	_ucycles = t._ucycles;
	_kcycles = t._kcycles;
}

std::istream& operator >>(std::istream& is,thread& t) {
	std::istream::size_type unlimited = std::numeric_limits<streamsize>::max();
	is.ignore(unlimited,' ') >> t._tid;
	is.ignore(unlimited,' ') >> t._pid;
	is.ignore(unlimited,' ') >> t._procName;
	is.ignore(unlimited,' ') >> std::ws;
	t._state = is.get() - '0';
	is.ignore(unlimited,' ') >> t._stackPages;
	is.ignore(unlimited,' ') >> t._schedCount;
	is.ignore(unlimited,' ') >> t._syscalls;
	is.setf(istream::hex);
	is.ignore(unlimited,' ') >> t._ucycles;
	is.ignore(unlimited,' ') >> t._kcycles;
	return is;
}
