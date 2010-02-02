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

#ifndef HEAP_H_
#define HEAP_H_

#include <esc/common.h>

/* the number of entries in the occupied map */
#define OCC_MAP_SIZE	512

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocates <size> bytes on the heap and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
void *malloc(u32 size);

/**
 * Allocates space for <num> elements, each <size> big, on the heap and memset's the area to 0.
 * If there is not enough memory the function returns NULL.
 *
 * @param num the number of elements
 * @param size the size of each element
 * @return the address of the memory or NULL
 */
void *calloc(u32 num,u32 size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
void *realloc(void *addr,u32 size);

/**
 * Frees the area starting at <addr>.
 */
void free(void *addr);

/**
 * Note that the heap does increase the data-pages of the process as soon as it's required and
 * does not decrease them. So the free-space may increase during runtime!
 *
 * @return the free space on the heap
 */
u32 heap_getFreeSpace(void);


#if DEBUGGING

/**
 * Prints the heap
 */
void heap_print(void);

#endif

#ifdef __cplusplus
}
#endif

#endif /* HEAP_H_ */
