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

#include "common.h"

/* a rectangle */
typedef struct {
	int x;
	int y;
	ushort width;
	ushort height;
	/* TODO remove! */
	ushort window;
} sRectangle;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Adds r2 to r1. That means r1 will enclose r1 and r2 after the call
 *
 * @param r1 the first rectangle
 * @param r2 the second rectangle
 */
void rectAdd(sRectangle *r1,sRectangle *r2);

/**
 * @param r the rectangle
 * @param x the x-coordinate
 * @param y the y-coordinate
 * @return whether the rectangle contains the given point
 */
bool rectContains(sRectangle *r,int x,int y);

/**
 * Substracts <r2> from <r1> and creates an array of rectangles that are parts of <r1> and not
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
 * NOTE: You have to free the memory using rectFree, if the return-value is not NULL!
 *
 * @param r1 the rectangle to split
 * @param r2 the rectangle to split by
 * @param rectCount will be set to the number of created rects
 * @return the rectangle array, allocated on the heap; NULL if rectCount = 0
 */
sRectangle **rectSubstract(sRectangle *r1,sRectangle *r2,size_t *rectCount) A_CHECKRET;

/**
 * Frees the given rectangles
 *
 * @param rects the rectangles created by rectSubstract (may be NULL)
 * @param count the number of rectangles
 */
void rectFree(sRectangle **rects,size_t count);

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
