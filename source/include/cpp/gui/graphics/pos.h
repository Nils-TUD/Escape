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
#include <gui/graphics/size.h>
#include <iostream>

namespace gui {
	/**
	 * Holds the x- and y-position of an object
	 */
	struct Pos {
		explicit Pos() : x(), y() {
		}
		explicit Pos(gpos_t _x,gpos_t _y) : x(_x), y(_y) {
		}

		void operator +=(const Size &s) {
			x += s.width;
			y += s.height;
		};
		void operator -=(const Size &s) {
			x -= s.width;
			y -= s.height;
		};

		friend std::ostream &operator <<(std::ostream &os, const Pos &p) {
			os << "(" << p.x << "," << p.y << ")";
			return os;
		};

		gpos_t x;
		gpos_t y;
	};

	static inline Pos minpos(const Pos &a,const Pos &b) {
		return Pos(std::min(a.x,b.x),std::min(a.y,b.y));
	}
	static inline Pos maxpos(const Pos &a,const Pos &b) {
		return Pos(std::max(a.x,b.x),std::max(a.y,b.y));
	}

	static inline Pos operator +(const Pos &a,const Pos &b) {
		return Pos(a.x + b.x,a.y + b.y);
	}
	static inline Pos operator -(const Pos &a,const Pos &b) {
		return Pos(a.x - b.x,a.y - b.y);
	}

	static inline Pos operator +(const Pos &a,const Size &s) {
		return Pos(a.x + s.width,a.y + s.height);
	}
	static inline Pos operator -(const Pos &a,const Size &s) {
		return Pos(a.x - s.width,a.y - s.height);
	}

	static inline bool operator ==(const Pos &a,const Pos &b) {
		return a.x == b.x && a.y == b.y;
	}
	static inline bool operator !=(const Pos &a,const Pos &b) {
		return !(a == b);
	}
}
