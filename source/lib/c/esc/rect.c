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

#include <esc/common.h>
#include <esc/rect.h>
#include <stdlib.h>
#include <string.h>

void rectAdd(sRectangle *r1,sRectangle *r2) {
	int x = r1->x, y = r1->y;
	r1->x = MIN(r1->x,r2->x);
	r1->y = MIN(r1->y,r2->y);
	r1->width = MAX(x + r1->width,r2->x + r2->width) - r1->x;
	r1->height = MAX(y + r1->height,r2->y + r2->height) - r1->y;
}

bool rectContains(sRectangle *r,int x,int y) {
	return x >= r->x && x < r->x + r->width &&
		y >= r->y && y < r->y + r->height;
}

sRectangle **rectSubstract(sRectangle *r1,sRectangle *r2,size_t *rectCount) {
	size_t i,orgCount,count = 0;
	sRectangle **res;
	bool other = false;
	bool p1in,p2in,p3in,p4in;
	bool op1in = false,op2in = false,op3in = false,op4in = false;
	bool top,right,bottom,left;
	/* first determine which points of r2 are in r1 */
	p1in = rectContains(r1,r2->x,r2->y);
	p2in = rectContains(r1,r2->x + r2->width - 1,r2->y);
	p3in = rectContains(r1,r2->x + r2->width - 1,r2->y + r2->height - 1);
	p4in = rectContains(r1,r2->x,r2->y + r2->height - 1);

	/* put a rectangle at a side if one point of a side is in r1 */
	top = p1in || p2in;
	right = p2in || p3in;
	bottom = p3in || p4in;
	left = p4in || p1in;

	/* no overlap ? */
	count = top + bottom + right + left;
	if(count == 0) {
		/* ok, no point of r2 is in r1. but maybe the other way around */
		op1in = rectContains(r2,r1->x,r1->y);
		op2in = rectContains(r2,r1->x + r1->width - 1,r1->y);
		op3in = rectContains(r2,r1->x + r1->width - 1,r1->y + r1->height - 1);
		op4in = rectContains(r2,r1->x,r1->y + r1->height - 1);

		/* first check if all points of r1 are in r2 */
		if(op1in && op2in && op3in && op4in) {
			/* if so, substracting r2 from r1 leaves nothing over */
			*rectCount = 0;
			return NULL;
		}

		/* if no points of r1 are in r2 and no points of r2 are in r1 the substraction
		 * is 2 rects */
		other = true;
		if(!op1in && !op2in && !op3in && !op4in) {
			if(!(r2->x >= r1->x && r2->x < r1->x + r1->width &&
					r2->y < r1->y && r2->y + r2->height >= r1->y + r1->height) &&
				!(r2->y >= r1->y && r2->y < r1->y + r1->height &&
					r2->x < r1->x && r2->x + r2->width >= r1->x + r1->width)) {
				/* no intersection */
				*rectCount = 0;
				return NULL;
			}
			count = 2;
		}
		else
			count = 1;
	}

	/* allocate memory */
	orgCount = count;
	res = (sRectangle**)malloc(count * sizeof(sRectangle*));
	if(res == NULL) {
		*rectCount = 0;
		return NULL;
	}
	for(i = 0; i < count; i++) {
		res[i] = (sRectangle*)malloc(sizeof(sRectangle));
		if(res[i] == 0) {
			rectFree(res,i);
			*rectCount = 0;
			return NULL;
		}
	}

	/* other way around, that means r1 in r2? */
	if(other) {
		if(count == 1) {
			count = 0;
			/* in this case we know that there are exactly 2 points of r1 in r2 */
			/* since 4 points would have returned above and 1 point is not possible because we
			 * would have a point of r2 in r1 */

			/* left 2 points */
			if(op1in && op4in) {
				res[count]->x = r2->x + r2->width;
				res[count]->y = r1->y;
				res[count]->width = (r1->x + r1->width) - res[count]->x;
				res[count]->height = r1->height;
			}
			/* right 2 points */
			else if(op2in && op3in) {
				res[count]->x = r1->x;
				res[count]->y = r1->y;
				res[count]->width = r2->x - r1->x;
				res[count]->height = r1->height;
			}
			/* top 2 points */
			else if(op1in && op2in) {
				res[count]->x = r1->x;
				res[count]->y = r2->y + r2->height;
				res[count]->width = r1->width;
				res[count]->height = (r1->y + r1->height) - res[count]->y;
			}
			else {
				/* bottom 2 points */
				res[count]->x = r1->x;
				res[count]->y = r1->y;
				res[count]->width = r1->width;
				res[count]->height = r2->y - res[count]->y;
			}

			/* we don't need empty rectangles */
			if(res[count]->width > 0 && res[count]->height > 0)
				count++;
		}
		else {
			count = 0;
			/* no edge-points intersect */

			if(r1->height > r2->height) {
				/* top */
				res[count]->x = r1->x;
				res[count]->y = r1->y;
				res[count]->width = r1->width;
				res[count]->height = r2->y - r1->y;
				if(res[count]->width > 0 && res[count]->height > 0)
					count++;

				/* bottom */
				res[count]->x = r1->x;
				res[count]->y = r2->y + r2->height;
				res[count]->width = r1->width;
				res[count]->height = (r1->y + r1->height) - res[count]->y;
				if(res[count]->width > 0 && res[count]->height > 0)
					count++;
			}
			else {
				/* left */
				res[count]->x = r1->x;
				res[count]->y = r1->y;
				res[count]->width = r2->x - r1->x;
				res[count]->height = r1->height;
				if(res[count]->width > 0 && res[count]->height > 0)
					count++;

				/* right */
				res[count]->x = r2->x + r2->width;
				res[count]->y = r1->y;
				res[count]->width = (r1->x + r1->width) - res[count]->x;
				res[count]->height = r1->height;
				if(res[count]->width > 0 && res[count]->height > 0)
					count++;
			}
		}
	}
	else {
		count = 0;
		if(top) {
			res[count]->x = r1->x;
			res[count]->y = r1->y;
			res[count]->width = r1->width;
			res[count]->height = r2->y - r1->y;
			if(res[count]->width > 0 && res[count]->height > 0)
				count++;
		}
		/* bottom */
		if(bottom) {
			res[count]->x = r1->x;
			res[count]->y = r2->y + r2->height;
			res[count]->width = r1->width;
			res[count]->height = (r1->y + r1->height) - res[count]->y;
			if(res[count]->width > 0 && res[count]->height > 0)
				count++;
		}
		/* left */
		if(left) {
			res[count]->x = r1->x;
			/* take care that the rectangle doesn't overlap the ones at top and bottom */
			if(top)
				res[count]->y = r2->y;
			else
				res[count]->y = r1->y;
			res[count]->width = r2->x - r1->x;
			if(top && bottom)
				res[count]->height = r2->height;
			else if(bottom)
				res[count]->height = (r2->y + r2->height) - res[count]->y;
			else
				res[count]->height = (r1->y + r1->height) - res[count]->y;
			if(res[count]->width > 0 && res[count]->height > 0)
				count++;
		}
		/* right */
		if(right) {
			res[count]->x = r2->x + r2->width;
			if(top)
				res[count]->y = r2->y;
			else
				res[count]->y = r1->y;
			res[count]->width = (r1->x + r1->width) - res[count]->x;
			if(top && bottom)
				res[count]->height = r2->height;
			else if(bottom)
				res[count]->height = (r2->y + r2->height) - res[count]->y;
			else
				res[count]->height = (r1->y + r1->height) - res[count]->y;
			if(res[count]->width > 0 && res[count]->height > 0)
				count++;
		}
	}

	/* free buffer if we have no rectangle */
	if(count == 0) {
		rectFree(res,orgCount);
		*rectCount = 0;
		return NULL;
	}

	*rectCount = count;
	return res;
}

void rectFree(sRectangle **rects,size_t count) {
	if(rects) {
		size_t i;
		for(i = 0; i < count; i++)
			free(rects[i]);
		free(rects);
	}
}

bool rectIntersect(sRectangle *r1,sRectangle *r2,sRectangle *intersect) {
	bool p1in,p2in,p3in,p4in;
	bool op1in,op2in,op3in,op4in;
	p1in = rectContains(r1,r2->x,r2->y);
	p2in = rectContains(r1,r2->x + r2->width - 1,r2->y);
	p3in = rectContains(r1,r2->x + r2->width - 1,r2->y + r2->height - 1);
	p4in = rectContains(r1,r2->x,r2->y + r2->height - 1);

	/* if all points of r2 are in r1, r2 is the intersection */
	if(p1in && p2in && p3in && p4in) {
		intersect->x = r2->x;
		intersect->y = r2->y;
		intersect->width = r2->width;
		intersect->height = r2->height;
		return true;
	}

	/* if all points of r2 are not in r1, maybe r1 is in r2 */
	if(!p1in && !p2in && !p3in && !p4in) {
		/* ok, no point of r2 is in r1. but maybe the other way around */
		op1in = rectContains(r2,r1->x,r1->y);
		op2in = rectContains(r2,r1->x + r1->width - 1,r1->y);
		op3in = rectContains(r2,r1->x + r1->width - 1,r1->y + r1->height - 1);
		op4in = rectContains(r2,r1->x,r1->y + r1->height - 1);

		/* first check if all points of r1 are in r2 */
		if(op1in && op2in && op3in && op4in) {
			intersect->x = r1->x;
			intersect->y = r1->y;
			intersect->width = r1->width;
			intersect->height = r1->height;
			return true;
		}

		/* no points of r1 are in r2 and no points of r2 are in r1 */
		if(!op1in && !op2in && !op3in && !op4in) {
			if(!(r2->x >= r1->x && r2->x < r1->x + r1->width &&
					r2->y < r1->y && r2->y + r2->height >= r1->y + r1->height) &&
				!(r2->y >= r1->y && r2->y < r1->y + r1->height &&
					r2->x < r1->x && r2->x + r2->width >= r1->x + r1->width)) {
				/* no intersection */
				return false;
			}

			/* vertical */
			if(r1->height > r2->height) {
				intersect->x = r1->x;
				intersect->y = r2->y;
				intersect->width = r1->width;
				intersect->height = (r2->y + r2->height) - intersect->y;
			}
			/* horizontal */
			else {
				intersect->x = r2->x;
				intersect->y = r1->y;
				intersect->width = (r2->x + r2->width) - intersect->x;
				intersect->height = r1->height;
			}
		}
		else {
			/* in this case we know that there are exactly 2 points of r1 in r2 */
			/* since 4 points would have returned above and 1 point is not possible because we
			 * would have a point of r2 in r1 */

			/* left 2 points */
			if(op1in && op4in) {
				intersect->x = r1->x;
				intersect->y = r1->y;
				intersect->width = (r2->x + r2->width) - intersect->x;
				intersect->height = r1->height;
			}
			/* right 2 points */
			else if(op2in && op3in) {
				intersect->x = r2->x;
				intersect->y = r1->y;
				intersect->width = (r1->x + r1->width) - r2->x;
				intersect->height = r1->height;
			}
			/* top 2 points */
			else if(op1in && op2in) {
				intersect->x = r1->x;
				intersect->y = r1->y;
				intersect->width = r1->width;
				intersect->height = (r2->y + r2->height) - intersect->y;
			}
			else {
				/* bottom 2 points */
				intersect->x = r1->x;
				intersect->y = r2->y;
				intersect->width = r1->width;
				intersect->height = (r1->y + r1->height) - intersect->y;
			}
		}

		return true;
	}

	/* now there are 2 or less points in r1 */

	/* left 2 points are in r1 */
	if(p1in && p4in) {
		intersect->x = r2->x;
		intersect->y = r2->y;
		intersect->width = (r1->x + r1->width) - r2->x;
		intersect->height = r2->height;
		return true;
	}
	/* right 2 points are in r1 */
	if(p2in && p3in) {
		intersect->x = r1->x;
		intersect->y = r2->y;
		intersect->width = (r2->x + r2->width) - r1->x;
		intersect->height = r2->height;
		return true;
	}
	/* top 2 points are in r1 */
	if(p1in && p2in) {
		intersect->x = r2->x;
		intersect->y = r2->y;
		intersect->width = r2->width;
		intersect->height = (r1->y + r1->height) - r2->y;
		return true;
	}
	/* bottom 2 points are in r1 */
	if(p4in && p3in) {
		intersect->x = r2->x;
		intersect->y = r1->y;
		intersect->width = r2->width;
		intersect->height = (r2->y + r2->height) - r1->y;
		return true;
	}

	/* 3 points in r1 is not possible, so exactly 1 point is in r1 now */
	if(p1in) {
		intersect->x = r2->x;
		intersect->y = r2->y;
		intersect->width = (r1->x + r1->width) - r2->x;
		intersect->height = (r1->y + r1->height) - r2->y;
		return true;
	}
	if(p2in) {
		intersect->x = r1->x;
		intersect->y = r2->y;
		intersect->width = (r2->x + r2->width) - r1->x;
		intersect->height = (r1->y + r1->height) - r2->y;
		return true;
	}
	if(p3in) {
		intersect->x = r1->x;
		intersect->y = r1->y;
		intersect->width = (r2->x + r2->width) - r1->x;
		intersect->height = (r2->y + r2->height) - r1->y;
		return true;
	}
	if(p4in) {
		intersect->x = r2->x;
		intersect->y = r1->y;
		intersect->width = (r1->x + r1->width) - r2->x;
		intersect->height = (r2->y + r2->height) - r1->y;
		return true;
	}

	/* never reached */
	return false;
}
