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
#include <esc/sllist.h>

/**
 * A function to calculate the key of the given data
 */
typedef ulong (*fGetKey)(const void *data);

typedef struct {
	/* array of linked lists as map */
	sSLList **map;
	/* number of linked lists */
	size_t mapSize;
	/* number of elements in the map */
	size_t elCount;
	/* the function to calculate the key of data */
	fGetKey keyFunc;
	/* for iterating: the current index in the map */
	size_t curIndex;
	/* for iterating: the current node in a linked list */
	sSLNode *curNode;
	/* for iterating: the current overall element-index */
	size_t curElIndex;
} sHashMap;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new hashmap
 *
 * @param map the map to use (expects an array of linked lists). HAS TO be zero'ed!
 * @param mapSize the number of linked lists
 * @param keyFunc the function to calculate the key of data
 * @return the created map
 */
sHashMap *hm_create(sSLList **map,size_t mapSize,fGetKey keyFunc);

/**
 * @param m the hashmap
 * @return the number of elements in the map
 */
size_t hm_getCount(sHashMap *m);

/**
 * Fetches the data for the given key
 *
 * @param m the hashmap
 * @param key the key of the data to find
 * @return the data or NULL if not found
 */
void *hm_get(sHashMap *m,ulong key);

/**
 * Adds (i.e. it will NOT replace data with the same key!) the given data to the map.
 * Note that the map assumes that each key is present just once in the map! So, you have to
 * take care for that!
 *
 * @param m the hashmap
 * @param data the data to add
 * @return true if successfull (may fail when not enough heap-mem is available)
 */
bool hm_add(sHashMap *m,const void *data);

/**
 * Removes the given data from the map
 *
 * @param m the hashmap
 * @param data the data to remove
 */
void hm_remove(sHashMap *m,const void *data);

/**
 * Moves the iterator to the beginning of the map. Required once before each iteration with
 * hm_next().
 *
 * @param m the hashmap
 */
void hm_begin(sHashMap *m);

/**
 * Moves the iterator to the next element and returns it. Please call hm_begin() before you
 * start to use this function!
 * Note that hm_next() and hm_begin() assume that you don't change the hashmap while you're
 * iterating through it!
 *
 * @param m the hashmap
 * @return the element or NULL if there is none
 */
void *hm_next(sHashMap *m);

/**
 * Destroys the given map. I.e. it free's the linked lists (without deleting the data in it) and
 * free's the map itself
 *
 * @param m the map
 */
void hm_destroy(sHashMap *m);

/**
 * Prints the given hashmap
 *
 * @param m the map
 */
void hm_print(sHashMap *m);

#ifdef __cplusplus
}
#endif
