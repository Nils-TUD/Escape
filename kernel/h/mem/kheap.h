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

#ifndef KHEAP_H_
#define KHEAP_H_

#include <common.h>
#include <assert.h>

/**
 * @return the number of used bytes
 */
u32 kheap_getUsedMem(void);

/**
 * @return the total number of bytes occupied (frames reserved; maybe not all in use atm)
 */
u32 kheap_getOccupiedMem(void);

/**
 * Note that this function is intended for debugging-purposes only!
 *
 * @return the number of free bytes
 */
u32 kheap_getFreeMem(void);

/**
 * @param addr the area-address
 * @return the size of the area at given address (0 if not found)
 */
u32 kheap_getAreaSize(void *addr);

/**
 * Allocates <size> bytes in kernel-space and returns the pointer to the beginning of
 * the allocated memory. If there is not enough memory the function returns NULL.
 *
 * @param size the number of bytes to allocate
 * @return the address of the memory or NULL
 */
void *kheap_alloc(u32 size);

/**
 * Allocates space for <num> elements, each <size> big, on the heap and memset's the area to 0.
 * If there is not enough memory the function returns NULL.
 *
 * @param num the number of elements
 * @param size the size of each element
 * @return the address of the memory or NULL
 */
void *kheap_calloc(u32 num,u32 size);

/**
 * Reallocates the area at given address to the given size. That means either your data will
 * be copied to a different address or your area will be resized.
 *
 * @param addr the address of your area
 * @param size the number of bytes your area should be resized to
 * @return the address (may be different) of your area or NULL if there is not enough mem
 */
void *kheap_realloc(void *addr,u32 size);

/**
 * Frees the via kmalloc() allocated area starting at <addr>.
 */
void kheap_free(void *addr);

#if DEBUGGING

/**
 * Enables/disables "allocate and free" prints
 *
 * @param enabled the new value
 */
void kheap_dbg_setAaFEnabled(bool enabled);

/**
 * Prints the kernel-heap data-structure
 */
void kheap_dbg_print(void);

#endif

#endif /* KHEAP_H_ */
