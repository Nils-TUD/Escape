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

#ifndef RECT_H_
#define RECT_H_

#include "common.h"

/* a rectangle */
typedef struct {
	s16 x;
	s16 y;
	u16 width;
	u16 height;
	/* TODO remove! */
	u16 window;
} sRectangle;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param r the rectangle
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @return wether the rectangle contains the given point
 */
bool rectContains(sRectangle *r,s16 x,s16 y);

/**
 * Splits <r1> by <r2> and creates an array of rectangles that are parts of <r1> and not
 * part of <r2>. For example:
 * +---------+ <- r1
 * |         |
 * |     +---|--+
 * |     |   |  |
 * +-----|---+  |
 *       |      |  <- r2
 *       +------+
 * would result in:
 * +---------+
 * |    1    |
 * +-----+---+
 * |  2  |
 * +-----+
 * That means you'll get the rectangles 1 and 2.
 *
 * NOTE: You have to free the memory, if the return-value is not NULL!
 *
 * @param r1 the rectangle to split
 * @param r2 the rectangle to split by
 * @param rectCount will be set to the number of created rects
 * @return the rectangle array, allocated on the heap; NULL if rectCount = 0
 */
sRectangle **rectSplit(sRectangle *r1,sRectangle *r2,u32 *rectCount) A_CHECKRET;

/**
 * Calculates the intersection of <r1> and <r2>.
 *
 * @param r1 the first rectangle
 * @param r2 the second rectangle
 * @param intersect will be set to the intersection
 * @return true if there is an intersection
 */
bool rectIntersect(sRectangle *r1,sRectangle *r2,sRectangle *intersect);

#ifdef __cplusplus
}
#endif

#endif /* RECT_H_ */
