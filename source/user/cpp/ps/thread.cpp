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

std::istream& operator >>(std::istream& is,thread& t) {
	std::istream::size_type unlimited = std::numeric_limits<streamsize>::max();
	is.ignore(unlimited,'\n');	// tid
	is.ignore(unlimited,'\n');	// pid
	is.ignore(unlimited,'\n');	// procname
	is.ignore(unlimited,'\n');	// state
	is.ignore(unlimited,'\n');	// prio
	is.ignore(unlimited,'\n');	// stack
	is.ignore(unlimited,'\n');	// schedcount
	is.ignore(unlimited,'\n');	// syscalls
	is.ignore(unlimited,' ') >> t._runtime;
	is.setf(istream::hex);
	is.ignore(unlimited,' ') >> t._cycles;
	return is;
}
