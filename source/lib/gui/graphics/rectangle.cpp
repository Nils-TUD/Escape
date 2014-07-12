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

#include <sys/common.h>
#include <sys/rect.h>
#include <gui/graphics/rectangle.h>
#include <algorithm>

namespace gui {
	Rectangle unify(const Rectangle &r1,const Rectangle &r2) {
		Rectangle res = r1;
		int x = r1._pos.x, y = r1._pos.y;
		res._pos.x = std::min(r1._pos.x,r2._pos.x);
		res._pos.y = std::min(r1._pos.y,r2._pos.y);
		res._size.width = std::max(x + r1._size.width,r2._pos.x + r2._size.width) - r1._pos.x;
		res._size.height = std::max(y + r1._size.height,r2._pos.y + r2._size.height) - r1._pos.y;
		return res;
	}

	Rectangle intersection(const Rectangle &r1,const Rectangle &r2) {
		sRectangle sr1,sr2,res;
		sr1.x = r1.getPos().x;
		sr1.y = r1.getPos().y;
		sr1.width = r1.getSize().width;
		sr1.height = r1.getSize().height;
		sr2.x = r2.getPos().x;
		sr2.y = r2.getPos().y;
		sr2.width = r2.getSize().width;
		sr2.height = r2.getSize().height;
		if(!rectIntersect(&sr1,&sr2,&res))
			return Rectangle();
		return Rectangle(Pos(res.x,res.y),Size(res.width,res.height));
	}
}
