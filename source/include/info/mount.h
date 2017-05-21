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

#pragma once

#include <esc/stream/istream.h>
#include <esc/stream/ostream.h>
#include <limits>
#include <stddef.h>
#include <string>
#include <vector>

namespace info {
	class mount;
	esc::IStream& operator >>(esc::IStream& is,mount& m);
	esc::OStream& operator <<(esc::OStream& os,const mount& m);

	class mount {
		friend esc::IStream& operator >>(esc::IStream& is,mount& m);
	public:
		enum Type {
			T_KERNEL,
			T_USER
		};

		static std::vector<mount*> get_list();

		explicit mount()
			: _device(), _path(), _type(), _root(), _perm() {
		}

		const std::string &device() const {
			return _device;
		}
		const std::string &path() const {
			return _path;
		}
		Type type() const {
			return _type;
		}
		ino_t root() const {
			return _root;
		}
		uint perm() const {
			return _perm;
		}

	private:
		std::string _device;
		std::string _path;
		Type _type;
		ino_t _root;
		uint _perm;
	};
}
