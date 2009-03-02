/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */


#ifdef IN_KERNEL
#	include "../../kernel/h/common.h"
#	include "../../kernel/h/kheap.h"
#	include "../../kernel/h/video.h"
/* for panic (ASSERT) */
#	include "../../kernel/h/util.h"
#	define sllprintf	vid_printf
#	define free(x)		kheap_free(x)
#	define malloc(x)	kheap_alloc(x)
#else
#	include "../../libc/h/common.h"
#	include "../../libc/h/heap.h"
#	include "../../libc/h/debug.h"
/* for exit (ASSERT) */
#	include "../../libc/h/proc.h"
#	define sllprintf debugf
#endif

#include "../h/sllist.h"

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
static sNode *sll_getNode(sSLList *list,u32 index);

sSLList *sll_create(void) {
	sList *l = malloc(sizeof(sList));
	if(l == NULL)
		return NULL;

	l->first = NULL;
	l->last = NULL;
	l->length = 0;
	return (sSLList*)l;
}

void sll_destroy(sSLList *list) {
	/* free nodes */
	sList *l = (sList*)list;
	sNode *nn,*n = l->first;

	ASSERT(list != NULL,"list == NULL");

	while(n != NULL) {
		nn = n->next;
		free(n);
		n = nn;
	}
	/* free list */
	free(list);
}

sSLNode *sll_begin(sSLList *list) {
	return (sSLNode*)sll_getNode(list,0);
}

sSLNode *sll_nodeAt(sSLList *list,u32 index) {
	return (sSLNode*)sll_getNode(list,index);
}

u32 sll_length(sSLList *list) {
	sList *l = (sList*)list;
	ASSERT(list != NULL,"list == NULL");
	return l->length;
}

s32 sll_indexOf(sSLList *list,void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first;
	s32 i;
	for(i = 0; n != NULL; i++) {
		if(n->data == data)
			return i;
		n = n->next;
	}
	return -1;
}

sSLNode *sll_nodeWith(sSLList *list,void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first;
	while(n != NULL) {
		if(n->data == data)
			return (sSLNode*)n;
		n = n->next;
	}
	return NULL;
}

void *sll_get(sSLList *list,u32 index) {
	return sll_getNode(list,index)->data;
}

void sll_set(sSLList *list,void *data,u32 index) {
	sNode *n;
	ASSERT(data != NULL,"data == NULL");
	n = sll_getNode(list,index);
	n->data = data;
}

bool sll_append(sSLList *list,void *data) {
	sList *l = (sList*)list;
	return sll_insert(list,data,l->length);
}

bool sll_insert(sSLList *list,void *data,u32 index) {
	sList *l = (sList*)list;
	sNode *nn,*n = l->first,*ln = NULL;

	ASSERT(list != NULL,"list == NULL");
	ASSERT(data != NULL,"data == NULL");

	/* walk to the desired position */
	if(index == l->length) {
		n = NULL;
		ln = l->last;
	}
	else {
		while(index-- > 0) {
			ln = n;
			n = n->next;
		}
	}

	/* allocate node? */
	nn = malloc(sizeof(sNode));
	if(nn == NULL)
		return false;

	/* insert */
	nn->data = data;
	if(ln != NULL)
		ln->next = nn;
	else
		l->first = nn;
	if(n != NULL)
		nn->next = n;
	else {
		l->last = nn;
		nn->next = NULL;
	}
	l->length++;

	return true;
}

void sll_removeNode(sSLList *list,sSLNode *node,sSLNode *prev) {
	sList *l = (sList*)list;
	sNode *n = (sNode*)node,*ln = (sNode*)prev;

	ASSERT(list != NULL,"list == NULL");
	ASSERT(node != NULL,"node == NULL");
	ASSERT(ln == NULL || ln->next == n,"<prev> is not the previous node of <node>!");

	/* remove */
	if(ln != NULL)
		ln->next = n->next;
	else
		l->first = n->next;
	if(n->next == NULL)
		l->last = ln;
	l->length--;

	/* free */
	free(n);
}

void sll_removeFirst(sSLList *list,void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;

	ASSERT(list != NULL,"list == NULL");

	if(data != NULL) {
		while(n != NULL) {
			if(n->data == data)
				break;
			ln = n;
			n = n->next;
		}
	}

	/* TODO keep that? */
	ASSERT(n != NULL,"Data 0x%x does not exist!",data);

	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
}

void sll_removeIndex(sSLList *list,u32 index) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;

	ASSERT(list != NULL,"list == NULL");

	/* walk to the desired position */
	while(index-- > 0) {
		ln = n;
		n = n->next;
	}

	/* TODO keep that? */
	ASSERT(n != NULL,"Index %d does not exist!",index);

	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
}

static sNode *sll_getNode(sSLList *list,u32 index) {
	sList *l = (sList*)list;
	sNode *n;

	ASSERT(list != NULL,"list == NULL");
	ASSERT(index <= l->length,"The index %d does not exist",index);

	/* is it the last one? */
	if(index == l->length - 1)
		return l->last;

	/* walk to position */
	n = l->first;
	while(index-- > 0)
		n = n->next;
	return n;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void sll_dbg_print(sSLList *list) {
	sList *l = (sList*)list;
	sNode *n = l->first;

	ASSERT(list != NULL,"list == NULL");

	sllprintf("Linked list @ 0x%x\n",list);
	while(n != NULL) {
		sllprintf("\t[0x%x] data=0x%x, next=0x%x\n",n,n->data,n->next);
		n = n->next;
	}
}

#endif
