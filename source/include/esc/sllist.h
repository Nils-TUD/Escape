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

#ifndef SLLIST_H_
#define SLLIST_H_

#include <esc/common.h>

/* a node in the list (public view) */
typedef struct sSLNode sSLNode;
struct sSLNode {
	/* the user should not be able to change them */
	sSLNode *const next;
	void *data;
};

/* callbacks for allocating and free'ing nodes */
typedef void *(*fNodeAlloc)(size_t size);
typedef void (*fNodeFree)(void *n);

/* our list (the user should not know about the internal structure) */
/*typedef void* sSLList;*/
/* but for debugging, it is helpful :) */
typedef struct {
	const fNodeAlloc falloc;
	const fNodeFree ffree;
	const sSLNode *const first;
	const sSLNode *const last;
	const size_t length;
} sSLList;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a new list with default-allocation (i.e. on heap with malloc/free)
 *
 * @return the list or NULL if there is not enough mem
 */
sSLList *sll_create(void);

/**
 * Creates a new list that calls <falloc> to allocate a new node and <ffree> to free a node
 *
 * @param falloc the allocate-function
 * @param ffree the free-function
 * @return the list
 */
sSLList *sll_createExtern(fNodeAlloc falloc,fNodeFree ffree);

/**
 * Inits the given list. This is usefull when allocating a list on the stack / in the
 * data-segment.
 *
 * @param l the list
 * @param falloc the allocate-function
 * @param ffree the free-function
 */
void sll_init(sSLList *l,fNodeAlloc falloc,fNodeFree ffree);

/**
 * Clones the given list
 *
 * @param list the list
 * @return a clone of it
 */
sSLList *sll_clone(const sSLList *list);

/**
 * Destroyes the given list
 *
 * @param list the list
 * @param freeData whether the data of the nodes should be free'd. Requires that they are allocated
 * 	on the heap
 */
void sll_destroy(sSLList *list,bool freeData);

/**
 * Removes all elements from the list
 *
 * @param list the list
 * @param freeData whether the data of the nodes should be free'd. Requires that they are allocated
 * 	on the heap
 */
void sll_clear(sSLList *list,bool freeData);

/**
 * @param list the list
 * @return the number of elements in the list
 */
size_t sll_length(const sSLList *list);

/**
 * Returns the first node in the list. That allows you to iterate through it:
 * <code>
 * sSLNode *n;
 * for(n = sll_begin(list); n != NULL; n = n->next) {
 *   do something with n->data
 * }
 * </code>
 *
 * @param list the list
 * @return the first node (NULL if the list is empty)
 */
sSLNode *sll_begin(const sSLList *list);

/**
 * Returns the node at given index. See sll_begin().
 * Note that the index HAS TO exist!
 *
 * @param list the list
 * @param index the index
 * @return the node at given index
 */
sSLNode *sll_nodeAt(const sSLList *list,size_t index);

/**
 * Determines the index of the given data
 *
 * @param list the list
 * @param data the data to search for
 * @return the index of the first matching node or -1 if not found
 */
ssize_t sll_indexOf(const sSLList *list,const void *data);

/**
 * Determines the node with given data
 *
 * @param list the list
 * @param data the data to search for
 * @return the first matching node or NULL if not found
 */
sSLNode *sll_nodeWith(const sSLList *list,const void *data);

/**
 * Searches for the element at given index. First and last one can be found in O(1).
 *
 * @param list the list
 * @param index the index
 * @return the data at the given index
 */
void *sll_get(const sSLList *list,size_t index);

/**
 * Searches for the element at given index and sets the data to the given one. First and last
 * one can be found in O(1).
 *
 * @param list the list
 * @param data the new data (NULL is not allowed!)
 * @param index the index
 */
void sll_set(sSLList *list,const void *data,size_t index);

/**
 * Appends the given data to the list. This can be done in O(1).
 *
 * @param list the list
 * @param data the data (NULL is not allowed!)
 * @return true if successfull (otherwise not enough mem)
 */
bool sll_append(sSLList *list,const void *data);

/**
 * Inserts the given data at the given index to the list. This can be done in O(1) for the first
 * and last index.
 *
 * @param list the list
 * @param data the data (NULL is not allowed!)
 * @param index the index
 * @return true if successfull (otherwise not enough mem)
 */
bool sll_insert(sSLList *list,const void *data,size_t index);

/**
 * Inserts the given data behind <prev>. <prev> may be NULL if you want to insert it at the
 * beginning. This can be done with O(1) since the prev and next node is known.
 *
 * @param list the list
 * @param prev the prev-node (may be NULL)
 * @param data the data (NULL is not allowed!)
 * @return true if successfull (otherwise not enough mem)
 */
bool sll_insertAfter(sSLList *list,sSLNode *prev,const void *data);

/**
 * Removes the given node from the list. This is a faster alternative to sll_removeIndex()
 * because it is not required to loop through the list.
 *
 * @param list the list
 * @param node the node to remove
 * @param prev the previous node of <node>. NULL if there is no previous
 */
void sll_removeNode(sSLList *list,sSLNode *node,sSLNode *prev);

/**
 * Removes the first node from the given list
 *
 * @param list the list
 * @return the data of the first node or NULL if the list is empty
 */
void *sll_removeFirst(sSLList *list);

/**
 * Removes the first found element with given data. If the data is NULL the first element will
 * be removed.
 *
 * @param list the list
 * @param data the data to search for
 * @return the index if something has been removed or -1
 */
ssize_t sll_removeFirstWith(sSLList *list,const void *data);

/**
 * Removes the given index from the list
 *
 * @param list the list
 * @param index the index
 * @return the data of the removed node or NULL if not found
 */
void *sll_removeIndex(sSLList *list,size_t index);

/**
 * Prints the given list
 *
 * @param list the list
 */
void sll_print(const sSLList *list);

#ifdef __cplusplus
}
#endif

#endif /* SLLIST_H_ */
