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
#include <iostream>

namespace gui {
	struct Pos;

	/**
	 * Holds the width and height of an object
	 */
	struct Size {
		explicit Size() : width(), height() {
		}
		explicit Size(gsize_t _width,gsize_t _height) : width(_width), height(_height) {
		}
		explicit Size(const Pos &pos);

		bool empty() const {
			return width == 0 || height == 0;
		}

		void operator += (const Size &size) {
			width += size.width;
			height += size.height;
		}
		void operator -= (const Size &size) {
			width -= size.width;
			height -= size.height;
		}

		friend std::ostream &operator <<(std::ostream &os, const Size &s) {
			os << s.width << "x" << s.height;
			return os;
		}

		gsize_t width;
		gsize_t height;
	};

	inline Size operator +(const Size &a,const Size &b) {
		return Size(a.width + b.width,a.height + b.height);
	}
	inline Size operator -(const Size &a,const Size &b) {
		return Size(a.width - b.width,a.height - b.height);
	}

	inline bool operator ==(const Size &a,const Size &b) {
		return a.width == b.width && a.height == b.height;
	}
	inline bool operator !=(const Size &a,const Size &b) {
		return !(a == b);
	}

	inline Size minsize(const Size &a,const Size &b) {
		return Size(std::min(a.width,b.width),std::min(a.height,b.height));
	}
	inline Size maxsize(const Size &a,const Size &b) {
		return Size(std::max(a.width,b.width),std::max(a.height,b.height));
	}

	inline Size subsize(const Size &a,const Size &b) {
		Size res = a - b;
		if(res.width > a.width)
			res.width = 0;
		if(res.height > a.height)
			res.height = 0;
		return res;
	}
}
