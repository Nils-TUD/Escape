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

#ifndef VECTOR_H_
#define VECTOR_H_

#include <esc/common.h>
#include <esc/algo.h>
#include <util/iterator.h>

/**
 * A convenience-foreach for the vector. For example:
 * sVector *myVector = vec_create(sizeof(u32));
 * u32 e;
 * vforeach(myVector,e)
 *   cout->format(cout,"%u ",e);
 */
#define vforeach(v,eName)	\
	sIterator __it##eName = vec_iterator(v); \
	while((__it##eName).hasNext(&__it##eName) && \
			(eName = *(__typeof__(eName)*)(__it##eName).next(&(__it##eName))))

typedef struct {
/* private: */
	void *_elements;
	u32 _size;			/* size of the memarea <elements> */
	u32 _elSize;		/* size of one element */

/* public: */
	u32 count;			/* number of elements in the vector */
} sVector;

/**
 * Creates a new vector with given element-size
 *
 * @param elSize the element-size
 * @return the vector
 */
sVector *vec_create(u32 elSize);

/**
 * Creates a new vector with given element-size and given initial size
 *
 * @param elSize the element-size
 * @param count the number of initial elements to reserve space for
 * @return the vector
 */
sVector *vec_createSize(u32 elSize,u32 count);

/**
 * Creates a new vector from the given one, i.e. copies all elements
 *
 * @param v the vector
 * @return the new vector
 */
sVector *vec_copy(const sVector *v);

/**
 * Returns an iterator (allocated on the stack, nothing to free) for the given vector
 *
 * @param v the vector
 * @return the iterator
 */
sIterator vec_iterator(sVector *v);

/**
 * Adds an integer to the vector. This makes just sense if your vector contains elements with
 * size <= sizeof(u32). This is just a convenience-function for:
 * u32 i = 1234;
 * vec_add(v,&i);
 *
 * @param v the vector
 * @param val the value
 */
void vec_addInt(sVector *v,u32 val);

/**
 * Adds the given element to the vector, i.e. the memory at <p> will be copied into the vector.
 *
 * @param v the vector
 * @param p the pointer to the element to insert
 */
void vec_add(sVector *v,const void *p);

/**
 * Inserts an integer at given index into the vector. This makes just sense if your vector contains
 * elements with size <= sizeof(u32). This is just a convenience-function for:
 * u32 i = 1234;
 * vec_insert(v,index,&i);
 *
 * @param v the vector
 * @param index the index
 * @param val the value
 */
void vec_insertInt(sVector *v,u32 index,u32 val);

/**
 * Inserts the given element at the given index, i.e. the memory at <p> will be copied into the
 * vector.
 *
 * @param v the vector
 * @param index the index
 * @param p the pointer to the element to insert
 */
void vec_insert(sVector *v,u32 index,const void *p);

/**
 * Removes the given integer from the vector. This makes just sense if your vector contains
 * elements with size <= sizeof(u32). This is just a convenience-function for:
 * u32 i = 1234;
 * vec_removeObj(v,&i);
 *
 * @param v the vector
 * @param val the value
 */
void vec_removeInt(sVector *v,u32 val);

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
void vec_remove(sVector *v,u32 start,u32 count);

/**
 * Searches for the given integer in the vector. This makes just sense if your vector contains
 * elements with size <= sizeof(u32). This is just a convenience-function for:
 * u32 i = 1234;
 * vec_find(v,&i);
 *
 * @param v the vector
 * @param val the value
 * @return the index or -1 if not found
 */
s32 vec_findInt(sVector *v,u32 val);

/**
 * Searches for the given element in the vector, i.e. the memory at <p> will be compared with all
 * elements and the first matching index will be returned
 *
 * @param v the vector
 * @param p the pointer to the element to find
 * @return the index or -1 if not found
 */
s32 vec_find(sVector *v,const void *p);

/**
 * Sorts the given vector with qsort() and the default-compare-function, that looks like this:
 * static int defCmp(const void *a,const void *b) {
 *   return memcmp(a,b,sortElSize);
 * }
 * Note that this function is currently not thread-safe!
 *
 * @param v the vector
 */
void vec_sort(sVector *v);

/**
 * Sorts the given vector with qsort() and the given compare-function
 *
 * @param the vector
 * @param cmp the compare-function
 */
void vec_sortCustom(sVector *v,fCompare cmp);

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

#endif /* VECTOR_H_ */
