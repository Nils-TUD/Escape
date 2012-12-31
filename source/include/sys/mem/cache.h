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

#include <sys/common.h>

/**
 * Allocates <size> bytes from the cache
 *
 * @param size the size
 * @return the pointer to the allocated area or NULL
 */
void *cache_alloc(size_t size);

/**
 * Allocates <num> objects with <size> bytes from the cache and zero's the memory.
 *
 * @param num the number of objects
 * @param size the size of an object
 * @return the pointer to the allocated area or NULL
 */
void *cache_calloc(size_t num,size_t size);

/**
 * Reallocates the area at <p> to be <size> bytes large. This may involve moving it to a different
 * location.
 *
 * @param p the area
 * @param size the new size of the area
 * @return the potentially new location of the area or NULL
 */
void *cache_realloc(void *p,size_t size);

/**
 * Frees the given area
 *
 * @param p the area
 */
void cache_free(void *p);

/**
 * @return the number of pages used by the cache
 */
size_t cache_getPageCount(void);

/**
 * @return the occupied memory
 */
size_t cache_getOccMem(void);

/**
 * @return the used memory
 */
size_t cache_getUsedMem(void);

/**
 * Prints the cache
 */
void cache_print(void);


#if DEBUGGING

/**
 * Enables/disables "allocate and free" prints
 *
 * @param enabled the new value
 */
void cache_dbg_setAaFEnabled(bool enabled);

#endif
