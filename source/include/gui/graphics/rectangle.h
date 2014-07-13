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

#include <sys/common.h>
#include <gui/graphics/pos.h>
#include <gui/graphics/size.h>
#include <iostream>
#include <vector>

namespace gui {
	class Rectangle;

	Rectangle unify(const Rectangle &r1,const Rectangle &r2);
	Rectangle intersection(const Rectangle &r1,const Rectangle &r2);
	std::vector<Rectangle> substraction(const Rectangle &r1,const Rectangle &r2);

	class Rectangle {
		friend Rectangle unify(const Rectangle &r1,const Rectangle &r2);
		friend std::vector<Rectangle> substraction(const Rectangle &r1,const Rectangle &r2);

	public:
		explicit Rectangle() : _pos(), _size() {
		}
		explicit Rectangle(const Pos &pos,const Size &size) : _pos(pos), _size(size) {
		}
		explicit Rectangle(gpos_t x,gpos_t y,gsize_t width,gsize_t height)
			: _pos(x,y), _size(width,height) {
		}

		gpos_t x() const {
			return _pos.x;
		}
		gpos_t y() const {
			return _pos.y;
		}
		gsize_t width() const {
			return _size.width;
		}
		gsize_t height() const {
			return _size.height;
		}

		Pos &getPos() {
			return _pos;
		}
		const Pos &getPos() const {
			return _pos;
		}
		void setPos(gpos_t x,gpos_t y) {
			setPos(Pos(x,y));
		}
		void setPos(const gui::Pos &p) {
			_pos = p;
		}
		Size &getSize() {
			return _size;
		}
		const Size &getSize() const {
			return _size;
		}
		void setSize(gsize_t width,gsize_t height) {
			setSize(Size(width,height));
		}
		void setSize(const gui::Size &s) {
			_size = s;
		}

		bool empty() const {
			return _size.empty();
		}

		bool contains(gpos_t _x,gpos_t _y) const {
			return _x >= x() && _x < (gpos_t)(x() + width()) &&
				_y >= y() && _y < (gpos_t)(y() + height());
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

	protected:
		Pos _pos;
		Size _size;
	};
}
