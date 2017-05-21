/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <esc/stream/fstream.h>
#include <info/mount.h>
#include <assert.h>

using namespace esc;

namespace info {
	std::vector<mount*> mount::get_list() {
		std::vector<mount*> list;
		FStream f("/sys/pid/self/ms/info","r");
		while(f.good()) {
			mount *m = new mount;
			f >> *m;
			if(!f.good()) {
				delete m;
				break;
			}
			list.push_back(m);
		}
		return list;
	}

	IStream& operator >>(IStream& is,mount& l) {
		size_t unlimited = std::numeric_limits<size_t>::max();

		is >> l._device;
		is.ignore(unlimited,' '); // "on"

		is >> l._path;
		is.ignore(unlimited,' '); // "type"

		std::string type;
		is >> type;
		l._type = type == "kernel" ? mount::T_KERNEL : mount::T_USER;

		is.get(); // '('

		l._perm = 0;
		if(is.get() == 'r')
			l._perm |= O_READ;
		if(is.get() == 'w')
			l._perm |= O_WRITE;
		if(is.get() == 'x')
			l._perm |= O_EXEC;

		if(is.get() == ',') {
			is.ignore(unlimited,'=');	// "rootino=""

			is >> l._root;
		}
		is.ignore(unlimited,'\n');
		return is;
	}

	OStream& operator <<(OStream& os,const mount& l) {
		os << "Mount[device=" << l.device() << ", path=" << l.path()
		   << ", type=" << (l.type() == mount::T_KERNEL ? "kernel" : "user")
		   << ", perm=" << l.perm() << ", root=" << l.root() << "]";
		return os;
	}
}
