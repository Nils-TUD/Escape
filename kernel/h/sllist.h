/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SLLIST_H_
#define SLLIST_H_

#include "common.h"

/* our list (the user should not know about the internal structure) */
typedef void* tSLList;

/* a node in the list (public view) */
typedef struct tSLNode tSLNode;
struct tSLNode {
	/* the user should not be able to change them */
	tSLNode *const next;
	void *const data;
};

/**
 * Creates a new list
 *
 * @return the list or NULL if there is not enough mem
 */
tSLList *sll_create(void);

/**
 * Destroyes the given list
 *
 * @param list the list
 */
void sll_destroy(tSLList *list);

/**
 * Prints the given list
 *
 * @param list the list
 */
void sll_print(tSLList *list);

/**
 * @param list the list
 * @return the number of elements in the list
 */
u32 sll_length(tSLList *list);

/**
 * Returns the first node in the list. That allows you to iterate through it:
 * <code>
 * tSLNode *n;
 * for(n = sll_begin(list); n != NULL; n = n->next) {
 *   \/* do something with n->data *\/
 * }
 * </code>
 *
 * @param list the list
 * @return the first node (NULL if the list is empty)
 */
tSLNode *sll_begin(tSLList *list);

/**
 * Returns the node at given index. See sll_begin().
 * Note that the index HAS TO exist!
 *
 * @param list the list
 * @param index the index
 * @return the node at given index
 */
tSLNode *sll_nodeAt(tSLList *list,u32 index);

/**
 * Searches for the element at given index. First and last one can be found in O(1).
 *
 * @param list the list
 * @param index the index
 * @return the data at the given index
 */
void *sll_get(tSLList *list,u32 index);

/**
 * Searches for the element at given index and sets the data to the given one. First and last
 * one can be found in O(1).
 *
 * @param list the list
 * @param data the new data (NULL is not allowed!)
 * @param index the index
 */
void sll_set(tSLList *list,void *data,u32 index);

/**
 * Appends the given data to the list. This can be done in O(1).
 *
 * @param list the list
 * @param data the data (NULL is not allowed!)
 * @return true if successfull (otherwise not enough mem)
 */
bool sll_append(tSLList *list,void *data);

/**
 * Inserts the given data at the given index to the list. This can be done in O(1) for the first
 * and last index.
 *
 * @param list the list
 * @param data the data (NULL is not allowed!)
 * @param index the index
 * @return true if successfull (otherwise not enough mem)
 */
bool sll_insert(tSLList *list,void *data,u32 index);

/**
 * Removes the given node from the list. This is a faster alternative to sll_removeIndex()
 * because it is not required to loop through the list.
 *
 * @param list the list
 * @param node the node to remove
 * @param prev the previous node of <node>. NULL if there is no previous
 */
void sll_removeNode(tSLList *list,tSLNode *node,tSLNode *prev);

/**
 * Removes the first found element with given data. If the data is NULL the first element will
 * be removed.
 *
 * @param list the list
 * @param data the data to search for
 */
void sll_removeFirst(tSLList *list,void *data);

/**
 * Removes the given index from the list
 *
 * @param list the list
 * @param index the index
 */
void sll_removeIndex(tSLList *list,u32 index);

#endif /* SLLIST_H_ */
