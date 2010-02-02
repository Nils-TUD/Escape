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

#ifndef COMMON_H_
#define COMMON_H_

#include <types.h>
#include <stddef.h>

/* file-number (in global file table) */
typedef s32 tFileNo;

#ifndef NDEBUG
#define DEBUGGING 1
#endif

/**
 * Assuming that <startx> < <endx> and <endx> is not included (that means with start=0 and end=10
 * 0 .. 9 is used), the macro determines wether the two ranges overlap anywhere.
 */
#define OVERLAPS(start1,end1,start2,end2) \
	(((start1) >= (start2) && (start1) < ((end2))) ||	/* start in range */	\
	((end1) > (start2) && (end1) <= (end2)) ||			/* end in range */		\
	((start1) < (start2) && (end1) > (end2)))			/* complete overlapped */

/* debugging */
#define DBG_PGCLONEPD(s)
#define DBG_KMALLOC(s)

#endif /*COMMON_H_*/
