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

#include "process.h"
#include "thread.h"

process::cycle_type process::cycles() const {
	if(_cycles == (cycle_type)-1) {
		_cycles = 0;
		for(std::vector<thread*>::const_iterator it = _threads.begin(); it != _threads.end(); ++it)
			_cycles += (*it)->cycles();
	}
	return _cycles;
}
process::cycle_type process::runtime() const {
	if(_runtime == (time_type)-1) {
		_runtime = 0;
		for(std::vector<thread*>::const_iterator it = _threads.begin(); it != _threads.end(); ++it)
			_runtime += (*it)->runtime();
	}
	return _runtime;
}

std::istream& operator >>(std::istream& is,process& p) {
	std::istream::size_type unlimited = std::numeric_limits<streamsize>::max();
	is.ignore(unlimited,' ') >> p._pid;
	is.ignore(unlimited,' ') >> p._ppid;
	is.ignore(unlimited,' ') >> p._uid;
	is.ignore(unlimited,' ') >> p._gid;
	is.ignore(unlimited,' ') >> p._cmd;
	is.ignore(unlimited,' ') >> p._pages;
	is.ignore(unlimited,' ') >> p._ownFrames;
	is.ignore(unlimited,' ') >> p._sharedFrames;
	is.ignore(unlimited,' ') >> p._swapped;
	is.ignore(unlimited,' ') >> p._input;
	is.ignore(unlimited,' ') >> p._output;
	return is;
}
