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

#ifndef ALGO_H_
#define ALGO_H_

#include <esc/common.h>

#ifdef __cplusplus
extern "C" {
#endif

/* function that compares <a> and <b> and returns:
 * 	<a> <  <b>: negative value
 *  <a> == <b>: 0
 *  <a> >  <b>: positive value
 *
 * @param a the first operand
 * @param b the second operand
 * @return the compare-result
 */
typedef int (*fCompare)(const void *a,const void *b);

/**
 * Searches the given key in the array pointed by base that is formed by num elements,
 * each of a size of size bytes, and returns a void* pointer to the first entry in the table
 * that matches the search key.
 *
 * To perform the search, the function compares the elements to the key by calling the function
 * comparator specified as the last argument.
 *
 * Because this function performs a binary search, the values in the base array are expected to be
 * already sorted in ascending order, with the same criteria used by comparator.
 *
 * @param key the key to search for
 * @param base the array to search
 * @param num the number of elements in the array
 * @param size the size of each element
 * @param cmp the compare-function
 * @return a pointer to an entry in the array that matches the search key or NULL if not found
 */
void *bsearch(const void *key,const void *base,u32 num,u32 size,fCompare cmp);

/**
 * Sorts the num elements of the array pointed by base, each element size bytes long, using the
 * comparator function to determine the order.
 * The sorting algorithm used by this function compares pairs of values by calling the specified
 * comparator function with two pointers to elements of the array.
 * The function does not return any value, but modifies the content of the array pointed by base
 * reordering its elements to the newly sorted order.
 *
 * @param base the array to sort
 * @param num the number of elements
 * @param size the size of each element
 * @param cmp the compare-function
 */
void qsort(void *base,u32 num,u32 size,fCompare cmp);

#ifdef __cplusplus
}
#endif

#endif /* ALGO_H_ */
