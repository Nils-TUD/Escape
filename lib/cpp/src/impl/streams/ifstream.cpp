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

#include <impl/streams/ifstream.h>

namespace esc {
	ifstream::ifstream()
		: istream(new filebuf()) {
	}
	ifstream::ifstream(const char* filename,ios_base::openmode which)
		: istream(new filebuf()) {
		rdbuf()->open(filename,which);
	}
	ifstream::~ifstream() {
	}

	filebuf* ifstream::rdbuf() const {
		return static_cast<filebuf*>(ios::rdbuf());
	}
	void ifstream::open(tFD fd,ios_base::openmode which) {
		rdbuf()->open(fd,which);
	}
	void ifstream::open(const char* s,ios_base::openmode mode) {
		rdbuf()->open(s,mode);
	}
	bool ifstream::is_open() const {
		return rdbuf()->is_open();
	}
	void ifstream::close() {
		rdbuf()->close();
	}
}
