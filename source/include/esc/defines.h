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

/* exit-codes */
#define EXIT_FAILURE			1
#define EXIT_SUCCESS			0

#define ARRAY_SIZE(array)		(sizeof((array)) / sizeof((array)[0]))
/* static strlen() */
#define SSTRLEN(str)			(sizeof((str)) - 1)

#define MAX(a,b)				((a) > (b) ? (a) : (b))
#define MIN(a,b)				((a) > (b) ? (b) : (a))

/**
 * Assuming that <startx> < <endx> and <endx> is not included (that means with start=0 and end=10
 * 0 .. 9 is used), the macro determines whether the two ranges overlap anywhere.
 */
#define OVERLAPS(start1,end1,start2,end2) \
	(((start1) >= (start2) && (start1) < ((end2))) ||	/* start in range */	\
	((end1) > (start2) && (end1) <= (end2)) ||			/* end in range */		\
	((start1) < (start2) && (end1) > (end2)))			/* complete overlapped */

/* gcc-attributes */
#define A_PACKED				__attribute__((packed))
#define A_ALIGNED(x)			__attribute__((aligned (x)))
#define A_CHECKRET				__attribute__((__warn_unused_result__))
#define A_NORETURN				__attribute__((noreturn))
#define A_INIT					__attribute__((section(".ctors")))
#define A_UNUSED				__attribute__((unused))
#define A_INLINE				__attribute__((inline))
