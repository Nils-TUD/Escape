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
#include <gui/graphics/rectangle.h>
#include <algorithm>

static void addRect(std::vector<gui::Rectangle> &rects,const gui::Rectangle &r) {
	/* we don't need empty rectangles */
	if(!r.empty())
		rects.push_back(r);
}

namespace gui {
	Rectangle unify(const Rectangle &r1,const Rectangle &r2) {
		Rectangle res = r1;
		int x = r1._pos.x, y = r1._pos.y;
		res._pos.x = std::min(r1._pos.x,r2._pos.x);
		res._pos.y = std::min(r1._pos.y,r2._pos.y);
		res._size.width = std::max(x + r1._size.width,r2._pos.x + r2._size.width) - res._pos.x;
		res._size.height = std::max(y + r1._size.height,r2._pos.y + r2._size.height) - res._pos.y;
		return res;
	}

	std::vector<Rectangle> substraction(const Rectangle &r1,const Rectangle &r2) {
		size_t count;
		std::vector<Rectangle> res;
		bool other = false;
		bool p1in,p2in,p3in,p4in;
		bool op1in = false,op2in = false,op3in = false,op4in = false;
		bool top,right,bottom,left;
		/* first determine which points of r2 are in r1 */
		p1in = r1.contains(r2.x(),r2.y());
		p2in = r1.contains(r2.x() + r2.width() - 1,r2.y());
		p3in = r1.contains(r2.x() + r2.width() - 1,r2.y() + r2.height() - 1);
		p4in = r1.contains(r2.x(),r2.y() + r2.height() - 1);

		/* put a rectangle at a side if one point of a side is in r1 */
		top = p1in || p2in;
		right = p2in || p3in;
		bottom = p3in || p4in;
		left = p4in || p1in;

		/* no overlap ? */
		count = top + bottom + right + left;
		if(count == 0) {
			/* ok, no point of r2 is in r1. but maybe the other way around */
			op1in = r2.contains(r1.x(),r1.y());
			op2in = r2.contains(r1.x() + r1.width() - 1,r1.y());
			op3in = r2.contains(r1.x() + r1.width() - 1,r1.y() + r1.height() - 1);
			op4in = r2.contains(r1.x(),r1.y() + r1.height() - 1);

			/* first check if all points of r1 are in r2 */
			if(op1in && op2in && op3in && op4in) {
				/* if so, substracting r2 from r1 leaves nothing over */
				return res;
			}

			/* if no points of r1 are in r2 and no points of r2 are in r1 the substraction
			 * is 2 rects */
			other = true;
			if(!op1in && !op2in && !op3in && !op4in) {
				if(!(r2.x() >= r1.x() && r2.x() < (int)(r1.x() + r1.width()) &&
						r2.y() < r1.y() && r2.y() + r2.height() >= r1.y() + r1.height()) &&
					!(r2.y() >= r1.y() && r2.y() < (int)(r1.y() + r1.height()) &&
						r2.x() < r1.x() && r2.x() + r2.width() >= r1.x() + r1.width())) {
					/* no intersection */
					return res;
				}
				count = 2;
			}
			else
				count = 1;
		}

		/* other way around, that means r1 in r2? */
		if(other) {
			if(count == 1) {
				/* in this case we know that there are exactly 2 points of r1 in r2 */
				/* since 4 points would have returned above and 1 point is not possible because we
				 * would have a point of r2 in r1 */

				/* left 2 points */
				if(op1in && op4in) {
					addRect(res,Rectangle(
						Pos(r2.x() + r2.width(),r1.y()),
						Size((r1.x() + r1.width()) - (r2.x() + r2.width()),r1.height())
					));
				}
				/* right 2 points */
				else if(op2in && op3in) {
					addRect(res,Rectangle(
						Pos(r1.x(),r1.y()),
						Size(r2.x() - r1.x(),r1.height())
					));
				}
				/* top 2 points */
				else if(op1in && op2in) {
					addRect(res,Rectangle(
						Pos(r1.x(),r2.y() + r2.height()),
						Size(r1.width(),(r1.y() + r1.height()) - (r2.y() + r2.height()))
					));
				}
				else {
					/* bottom 2 points */
					addRect(res,Rectangle(
						Pos(r1.x(),r1.y()),
						Size(r1.width(),r2.y() - r1.y())
					));
				}
			}
			else {
				/* no edge-points intersect */

				if(r1.height() > r2.height()) {
					/* top */
					addRect(res,Rectangle(
						Pos(r1.x(),r1.y()),
						Size(r1.width(),r2.y() - r1.y())
					));

					/* bottom */
					addRect(res,Rectangle(
						Pos(r1.x(),r2.y() + r2.height()),
						Size(r1.width(),(r1.y() + r1.height()) - (r2.y() + r2.height()))
					));
				}
				else {
					/* left */
					addRect(res,Rectangle(
						Pos(r1.x(),r1.y()),
						Size(r2.x() - r1.x(),r1.height())
					));

					/* right */
					addRect(res,Rectangle(
						Pos(r2.x() + r2.width(),r1.y()),
						Size((r1.x() + r1.width()) - (r2.x() + r2.width()),r1.height())
					));
				}
			}
		}
		else {
			if(top) {
				addRect(res,Rectangle(
					Pos(r1.x(),r1.y()),
					Size(r1.width(),r2.y() - r1.y())
				));
			}
			if(bottom) {
				addRect(res,Rectangle(
					Pos(r1.x(),r2.y() + r2.height()),
					Size(r1.width(),(r1.y() + r1.height()) - (r2.y() + r2.height()))
				));
			}
			if(left) {
				Rectangle nr;
				nr._pos.x = r1.x();
				/* take care that the rectangle doesn't overlap the ones at top and bottom */
				if(top)
					nr._pos.y = r2.y();
				else
					nr._pos.y = r1.y();
				nr._size.width = r2.x() - r1.x();
				if(top && bottom)
					nr._size.height = r2.height();
				else if(bottom)
					nr._size.height = (r2.y() + r2.height()) - nr.y();
				else
					nr._size.height = (r1.y() + r1.height()) - nr.y();
				addRect(res,nr);
			}
			/* right */
			if(right) {
				Rectangle nr;
				nr._pos.x = r2.x() + r2.width();
				if(top)
					nr._pos.y = r2.y();
				else
					nr._pos.y = r1.y();
				nr._size.width = (r1.x() + r1.width()) - nr.x();
				if(top && bottom)
					nr._size.height = r2.height();
				else if(bottom)
					nr._size.height = (r2.y() + r2.height()) - nr.y();
				else
					nr._size.height = (r1.y() + r1.height()) - nr.y();
				addRect(res,nr);
			}
		}

		return res;
	}

	Rectangle intersection(const Rectangle &r1,const Rectangle &r2) {
		bool p1in,p2in,p3in,p4in;
		bool op1in,op2in,op3in,op4in;
		p1in = r1.contains(r2.x(),r2.y());
		p2in = r1.contains(r2.x() + r2.width() - 1,r2.y());
		p3in = r1.contains(r2.x() + r2.width() - 1,r2.y() + r2.height() - 1);
		p4in = r1.contains(r2.x(),r2.y() + r2.height() - 1);

		/* if all points of r2 are in r1, r2 is the intersection */
		if(p1in && p2in && p3in && p4in)
			return r2;

		/* if all points of r2 are not in r1, maybe r1 is in r2 */
		if(!p1in && !p2in && !p3in && !p4in) {
			/* ok, no point of r2 is in r1. but maybe the other way around */
			op1in = r2.contains(r1.x(),r1.y());
			op2in = r2.contains(r1.x() + r1.width() - 1,r1.y());
			op3in = r2.contains(r1.x() + r1.width() - 1,r1.y() + r1.height() - 1);
			op4in = r2.contains(r1.x(),r1.y() + r1.height() - 1);

			/* first check if all points of r1 are in r2 */
			if(op1in && op2in && op3in && op4in)
				return r1;

			/* no points of r1 are in r2 and no points of r2 are in r1 */
			if(!op1in && !op2in && !op3in && !op4in) {
				if(!(r2.x() >= r1.x() && r2.x() < (int)(r1.x() + r1.width()) &&
						r2.y() < r1.y() && r2.y() + r2.height() >= r1.y() + r1.height()) &&
					!(r2.y() >= r1.y() && r2.y() < (int)(r1.y() + r1.height()) &&
						r2.x() < r1.x() && r2.x() + r2.width() >= r1.x() + r1.width())) {
					/* no intersection */
					return Rectangle();
				}

				/* vertical */
				if(r1.height() > r2.height()) {
					return Rectangle(
						Pos(r1.x(),r2.y()),
						Size(r1.width(),(r2.y() + r2.height()) - r2.y())
					);
				}
				/* horizontal */
				else {
					return Rectangle(
						Pos(r2.x(),r1.y()),
						Size((r2.x() + r2.width()) - r2.x(),r1.height())
					);
				}
			}
			else {
				/* in this case we know that there are exactly 2 points of r1 in r2 */
				/* since 4 points would have returned above and 1 point is not possible because we
				 * would have a point of r2 in r1 */

				/* left 2 points */
				if(op1in && op4in) {
					return Rectangle(
						Pos(r1.x(),r1.y()),
						Size((r2.x() + r2.width()) - r1.x(),r1.height())
					);
				}
				/* right 2 points */
				else if(op2in && op3in) {
					return Rectangle(
						Pos(r2.x(),r1.y()),
						Size((r1.x() + r1.width()) - r2.x(),r1.height())
					);
				}
				/* top 2 points */
				else if(op1in && op2in) {
					return Rectangle(
						Pos(r1.x(),r1.y()),
						Size(r1.width(),(r2.y() + r2.height()) - r1.y())
					);
				}
				else {
					/* bottom 2 points */
					return Rectangle(
						Pos(r1.x(),r2.y()),
						Size(r1.width(),(r1.y() + r1.height()) - r2.y())
					);
				}
			}
		}

		/* now there are 2 or less points in r1 */

		/* left 2 points are in r1 */
		if(p1in && p4in) {
			return Rectangle(
				Pos(r2.x(),r2.y()),
				Size((r1.x() + r1.width()) - r2.x(),r2.height())
			);
		}
		/* right 2 points are in r1 */
		if(p2in && p3in) {
			return Rectangle(
				Pos(r1.x(),r2.y()),
				Size((r2.x() + r2.width()) - r1.x(),r2.height())
			);
		}
		/* top 2 points are in r1 */
		if(p1in && p2in) {
			return Rectangle(
				Pos(r2.x(),r2.y()),
				Size(r2.width(),(r1.y() + r1.height()) - r2.y())
			);
		}
		/* bottom 2 points are in r1 */
		if(p4in && p3in) {
			return Rectangle(
				Pos(r2.x(),r1.y()),
				Size(r2.width(),(r2.y() + r2.height()) - r1.y())
			);
		}

		/* 3 points in r1 is not possible, so exactly 1 point is in r1 now */
		if(p1in) {
			return Rectangle(
				Pos(r2.x(),r2.y()),
				Size((r1.x() + r1.width()) - r2.x(),(r1.y() + r1.height()) - r2.y())
			);
		}
		if(p2in) {
			return Rectangle(
				Pos(r1.x(),r2.y()),
				Size((r2.x() + r2.width()) - r1.x(),(r1.y() + r1.height()) - r2.y())
			);
		}
		if(p3in) {
			return Rectangle(
				Pos(r1.x(),r1.y()),
				Size((r2.x() + r2.width()) - r1.x(),(r2.y() + r2.height()) - r1.y())
			);
		}

		assert(p4in);
		return Rectangle(
			Pos(r2.x(),r1.y()),
			Size((r1.x() + r1.width()) - r2.x(),(r2.y() + r2.height()) - r1.y())
		);
	}
}
