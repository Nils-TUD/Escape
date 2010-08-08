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

#include <impl/streams/fstream.h>

namespace std {
	fstream::fstream()
		: iostream(new filebuf()) {
	}
	fstream::fstream(const char* filename,ios_base::openmode which)
		: iostream(new filebuf()) {
		rdbuf()->open(filename,which);
	}
	fstream::~fstream() {
	}

	filebuf* fstream::rdbuf() const {
		return static_cast<filebuf*>(ios::rdbuf());
	}
	void fstream::open(const char* s,ios_base::openmode mode) {
		rdbuf()->open(s,mode);
	}
	bool fstream::is_open() const {
		return rdbuf()->is_open();
	}
	void fstream::close() {
		rdbuf()->close();
	}
}
