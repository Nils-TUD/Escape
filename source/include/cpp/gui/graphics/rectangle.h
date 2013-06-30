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

#pragma once

#include <esc/common.h>
#include <gui/graphics/pos.h>
#include <gui/graphics/size.h>
#include <iostream>

namespace gui {
	class Rectangle {
	public:
		explicit Rectangle() : _pos(), _size() {
		}
		explicit Rectangle(const Pos &pos,const Size &size) : _pos(pos), _size(size) {
		}

		const Pos &getPos() const {
			return _pos;
		}
		const Size &getSize() const {
			return _size;
		}
		bool empty() const {
			return _size.empty();
		}

		void operator += (const Size &size) {
			_size += size;
		}
		void operator -= (const Size &size) {
			_size -= size;
		}

		friend std::ostream &operator <<(std::ostream &os, const Rectangle &r) {
			os << "Rect[@" << r._pos << ", " << r._size << "]";
			return os;
		}

		Pos _pos;
		Size _size;
	};

	Rectangle intersection(const Rectangle &r1,const Rectangle &r2);
}
