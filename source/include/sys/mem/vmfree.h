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

typedef struct sVMFreeArea {
	uintptr_t addr;
	size_t size;
	struct sVMFreeArea *next;
} sVMFreeArea;

typedef struct {
	sVMFreeArea *list;
} sVMFreeMap;

/**
 * Inits the vmfree-map
 *
 * @param map the map
 * @param addr the address of the map
 * @param size the size of the map
 * @return true if successfull
 */
bool vmfree_init(sVMFreeMap *map,uintptr_t addr,size_t size);

/**
 * Destroys the given map
 *
 * @param map the map
 */
void vmfree_destroy(sVMFreeMap *map);

/**
 * Allocates an area in the given map, that is <size> bytes large.
 *
 * @param map the map
 * @param size the size of the area
 * @return the address of 0 if failed
 */
uintptr_t vmfree_allocate(sVMFreeMap *map,size_t size);

/**
 * Allocates an area in the given map at the specified address, that is <size> bytes large.
 *
 * @param map the map
 * @param addr the address of the area
 * @param size the size of the area
 * @return the address of 0 if failed
 */
bool vmfree_allocateAt(sVMFreeMap *map,uintptr_t addr,size_t size);

/**
 * Frees the area at <addr> with <size> bytes.
 *
 * @param map the map
 * @param addr the address of the area
 * @param size the size of the area
 */
void vmfree_free(sVMFreeMap *map,uintptr_t addr,size_t size);

/**
 * Just for debugging/testing: Determines the total number of free bytes in the map
 *
 * @param map the map
 * @param areas will be set to the number of areas in the map
 * @return the free bytes
 */
size_t vmfree_getSize(sVMFreeMap *map,size_t *areas);

/**
 * Prints the given map
 *
 * @param map the map
 */
void vmfree_print(sVMFreeMap *map);
