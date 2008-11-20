/**
 * @version		$Id: main.c 54 2008-11-15 15:07:46Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/sllist.h"
#include "../h/kheap.h"
#include "../h/util.h"
#include "../h/video.h"

/* a node in a list */
typedef struct tNode tNode;
struct tNode {
	tNode *next;
	void *data;
};

/* represents a list */
typedef struct {
	tNode *first;
	tNode *last;
	u32 length;
} tList;

/**
 * Searches for the node at given index
 *
 * @param list the list
 * @param index the index
 * @return the node
 */
static tNode *sll_getNode(tSLList *list,u32 index);

tSLList *sll_create(void) {
	tList *l = kheap_alloc(sizeof(tList));
	if(l == NULL)
		return NULL;

	l->first = NULL;
	l->last = NULL;
	l->length = 0;
	return (tSLList*)l;
}

void sll_destroy(tSLList *list) {
	/* free nodes */
	tList *l = (tList*)list;
	tNode *nn,*n = l->first;
	while(n != NULL) {
		nn = n->next;
		kheap_free(n);
		n = nn;
	}
	/* free list */
	kheap_free(list);
}

void sll_print(tSLList *list) {
	tList *l = (tList*)list;
	tNode *n = l->first;
	vid_printf("Linked list @ 0x%x\n",list);
	while(n != NULL) {
		vid_printf("\t[0x%x] data=0x%x, next=0x%x\n",n,n->data,n->next);
		n = n->next;
	}
}

tSLNode *sll_begin(tSLList *list) {
	return (tSLNode*)sll_getNode(list,0);
}

tSLNode *sll_nodeAt(tSLList *list,u32 index) {
	return (tSLNode*)sll_getNode(list,index);
}

u32 sll_length(tSLList *list) {
	tList *l = (tList*)list;
	return l->length;
}

void *sll_get(tSLList *list,u32 index) {
	return sll_getNode(list,index)->data;
}

void sll_set(tSLList *list,void *data,u32 index) {
	tNode *n;
	if(data == NULL)
		panic("data has to be != NULL");

	n = sll_getNode(list,index);
	n->data = data;
}

bool sll_append(tSLList *list,void *data) {
	tList *l = (tList*)list;
	return sll_insert(list,data,l->length);
}

bool sll_insert(tSLList *list,void *data,u32 index) {
	tList *l = (tList*)list;
	tNode *nn,*n = l->first,*ln = NULL;
	if(data == NULL)
		panic("data has to be != NULL");

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
	nn = kheap_alloc(sizeof(tNode));
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

void sll_removeNode(tSLList *list,tSLNode *node,tSLNode *prev) {
	tList *l = (tList*)list;
	tNode *n = (tNode*)node,*ln = (tNode*)prev;

	if(ln != NULL && ln->next != n)
		panic("<prev> is not the previous node of <node>!");

	/* remove */
	if(ln != NULL)
		ln->next = n->next;
	else
		l->first = n->next;
	if(n->next == NULL)
		l->last = ln;
	l->length--;

	/* free */
	kheap_free(n);
}

void sll_removeFirst(tSLList *list,void *data) {
	tList *l = (tList*)list;
	tNode *n = l->first,*ln = NULL;
	if(data != NULL) {
		while(n != NULL) {
			if(n->data == data)
				break;
			ln = n;
			n = n->next;
		}
	}

	/* TODO keep that? */
	if(n == NULL)
		panic("Data 0x%x does not exist!",data);

	sll_removeNode(list,(tSLNode*)n,(tSLNode*)ln);
}

void sll_removeIndex(tSLList *list,u32 index) {
	tList *l = (tList*)list;
	tNode *n = l->first,*ln = NULL;
	/* walk to the desired position */
	while(index-- > 0) {
		ln = n;
		n = n->next;
	}

	if(n == NULL)
		panic("Index %d does not exist!",index);

	sll_removeNode(list,(tSLNode*)n,(tSLNode*)ln);
}

static tNode *sll_getNode(tSLList *list,u32 index) {
	tList *l = (tList*)list;
	tNode *n;
	/* invalid index? */
	if(index > l->length)
		panic("The index %d does not exist",index);

	/* is it the last one? */
	if(index == l->length - 1)
		return l->last;

	/* walk to position */
	n = l->first;
	while(index-- > 0)
		n = n->next;
	return n;
}
