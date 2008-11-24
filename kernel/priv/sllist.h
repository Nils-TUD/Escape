/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVSLLIST_H_
#define PRIVSLLIST_H_

#include "../pub/common.h"
#include "../pub/sllist.h"

/* a node in a list */
typedef struct sNode sNode;
struct sNode {
	sNode *next;
	void *data;
};

/* represents a list */
typedef struct {
	sNode *first;
	sNode *last;
	u32 length;
} sList;

/**
 * Searches for the node at given index
 *
 * @param list the list
 * @param index the index
 * @return the node
 */
static sNode *sll_gesNode(sSLList *list,u32 index);

#endif /* PRIVSLLIST_H_ */
