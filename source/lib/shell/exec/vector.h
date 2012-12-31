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

#include <esc/common.h>
#include <stdlib.h>

typedef struct {
/* private: */
	void *_elements;
	size_t _size;			/* size of the memarea <elements> */
	size_t _elSize;		/* size of one element */

/* public: */
	size_t count;			/* number of elements in the vector */
} sVector;

/**
 * Creates a new vector with given element-size
 *
 * @param elSize the element-size
 * @return the vector
 */
sVector *vec_create(size_t elSize);

/**
 * Creates a new vector with given element-size and given initial size
 *
 * @param elSize the element-size
 * @param count the number of initial elements to reserve space for
 * @return the vector
 */
sVector *vec_createSize(size_t elSize,size_t count);

/**
 * Creates a new vector from the given one, i.e. copies all elements
 *
 * @param v the vector
 * @return the new vector
 */
sVector *vec_copy(const sVector *v);

/**
 * Returns the element with index <i>
 *
 * @param v the vector
 * @param i the index
 * @return the element
 */
void *vec_get(sVector *v,size_t i);

/**
 * Adds the given element to the vector, i.e. the memory at <p> will be copied into the vector.
 *
 * @param v the vector
 * @param p the pointer to the element to insert
 */
void vec_add(sVector *v,const void *p);

/**
 * Sets the value at <index> to <p>.
 *
 * @param v the vector
 * @param index the index
 * @param p the value
 */
void vec_set(sVector *v,size_t index,const void *p);

/**
 * Inserts the given element at the given index, i.e. the memory at <p> will be copied into the
 * vector.
 *
 * @param v the vector
 * @param index the index
 * @param p the pointer to the element to insert
 */
void vec_insert(sVector *v,size_t index,const void *p);

/**
 * Removes the given object from the vector, i.e. the memory at <p> will be compared with all
 * elements and the first matching one will be removed.
 *
 * @param v the vector
 * @param p the pointer to the element to remove
 */
void vec_removeObj(sVector *v,const void *p);

/**
 * Removes the given range from the vector
 *
 * @param v the vector
 * @param start the start-position
 * @param count the number of elements to remove
 */
void vec_remove(sVector *v,size_t start,size_t count);

/**
 * Searches for the given element in the vector, i.e. the memory at <p> will be compared with all
 * elements and the first matching index will be returned
 *
 * @param v the vector
 * @param p the pointer to the element to find
 * @return the index or -1 if not found
 */
ssize_t vec_find(sVector *v,const void *p);

/**
 * Destroys the given vector
 *
 * @param v the vector
 * @param freeElements if true, the function assumes that the elements have been allocated with
 * 	heap_alloc() or similar and therefore free's them with heap_free().
 */
void vec_destroy(sVector *v,bool freeElements);


#if DEBUGGING

/**
 * Prints the given vector
 *
 * @param v the vector
 */
void vec_dbg_print(sVector *v);

#endif
