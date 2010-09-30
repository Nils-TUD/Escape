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

#ifndef DYNARRAY_H_
#define DYNARRAY_H_

#include <sys/common.h>

/* This module is intended to provide a dynamically extending region. That means you have a limited
 * area in virtual-memory, but allocate the pages in it when needed. This way we still have a
 * "real" array, i.e. you can use the index-operator to get an object from that area and - most
 * important - can use the index as id to quickly get an object by id. This is of course much faster
 * than a hashmap or similar. So it combines the advantages of data-structures where each element
 * is allocated on the heap (space-efficient) and hashmaps, that provide a quick access by a key.
 * Of course there are also disadvantages:
 * - it is still limited in virtual-memory. but we have enough of it atm, so thats not really a
 *   problem.
 * - in this array may be multiple unused objects. which make an iteration over all objects
 *   a bit slower than e.g. a linked list.
 * - atm we don't free frames, even if no objects are in it anymore. The reason is that its not
 *   that easy to determine when a complete frame is unused. At least it would make this concept
 *   far more difficult. And I think, in "normal" usages of escape, this will never be a problem.
 *
 * Note that using the heap for that (with realloc) is a bad idea, because if anybody stores a
 * pointer to an object somewhere and the address changes because of a realloc, we have a problem...
 *
 * Atm this is just used for vfs-nodes and the GFT. So, at places where we need an indexable array
 * and where we usally need only a few objects, but neither want to have a low limit nor want to
 * have a huge static array.
 */

/* describes the array */
typedef struct {
	/* number of objects currently avaiable */
	u32 objCount;
	u32 objSize;
	/* the virtual-memory-area in which the array is */
	u32 areaBegin;
	u32 areaSize;
	/* number of pages currently used */
	u32 pageCount;
} sDynArray;

/**
 * Initializes the given dynamic array
 *
 * @param d the dynamic array
 * @param objSize the size of one object
 * @param areaBegin the beginning of the area in virtual memory where the objects should be
 * @param areaSize the size of that area
 */
void dyna_init(sDynArray *d,u32 objSize,u32 areaBegin,u32 areaSize);

/**
 * Extends the given array. That means, it allocates one page more and changes the number of
 * available objects correspondingly (objCount).
 *
 * @param d the dynamic array
 * @return true if successfull
 */
bool dyna_extend(sDynArray *d);

/**
 * Destroys the given dynamic array
 *
 * @param d the dynamic array
 */
void dyna_destroy(sDynArray *d);

#endif /* DYNARRAY_H_ */
