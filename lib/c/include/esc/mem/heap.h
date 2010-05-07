/**
 * $Id: heap.h 641 2010-05-02 18:24:30Z nasmussen $
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

#ifndef ESCHEAP_H_
#define ESCHEAP_H_

#include <esc/common.h>

/**
 * Allocates <size> bytes on the heap. Uses malloc(), but throws an exception on failure
 *
 * @param size the size
 * @return a pointer to the allocated mem
 */
void *heap_alloc(u32 size);

/**
 * Increases/decreases the given memory-area. Uses realloc(), but throws an exception on failure
 *
 * @param p the memory-area (NULL is the same as heap_alloc(<size))
 * @param size the size
 * @return a pointer to the allocated mem
 */
void *heap_realloc(void *p,u32 size);

/**
 * Allocates <size> bytes on the heap and fills them with zeros. Uses calloc(), but throws an
 * exception on failure
 *
 * @param size the size
 * @return a pointer to the allocated mem
 */
void *heap_calloc(u32 size);

/**
 * Free's the given memory. Uses free()
 *
 * @param p the memory
 */
void heap_free(void *p);

#endif /* ESCHEAP_H_ */
