/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../pub/common.h"
#include "../pub/kheap.h"
#include "../pub/util.h"
#include "../pub/video.h"

#include "../priv/sllist.h"

sSLList *sll_create(void) {
	sList *l = kheap_alloc(sizeof(sList));
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
	while(n != NULL) {
		nn = n->next;
		kheap_free(n);
		n = nn;
	}
	/* free list */
	kheap_free(list);
}

void sll_print(sSLList *list) {
	sList *l = (sList*)list;
	sNode *n = l->first;
	vid_printf("Linked list @ 0x%x\n",list);
	while(n != NULL) {
		vid_printf("\t[0x%x] data=0x%x, next=0x%x\n",n,n->data,n->next);
		n = n->next;
	}
}

sSLNode *sll_begin(sSLList *list) {
	return (sSLNode*)sll_gesNode(list,0);
}

sSLNode *sll_nodeAt(sSLList *list,u32 index) {
	return (sSLNode*)sll_gesNode(list,index);
}

u32 sll_length(sSLList *list) {
	sList *l = (sList*)list;
	return l->length;
}

void *sll_get(sSLList *list,u32 index) {
	return sll_gesNode(list,index)->data;
}

void sll_set(sSLList *list,void *data,u32 index) {
	sNode *n;
	if(data == NULL)
		panic("data has to be != NULL");

	n = sll_gesNode(list,index);
	n->data = data;
}

bool sll_append(sSLList *list,void *data) {
	sList *l = (sList*)list;
	return sll_insert(list,data,l->length);
}

bool sll_insert(sSLList *list,void *data,u32 index) {
	sList *l = (sList*)list;
	sNode *nn,*n = l->first,*ln = NULL;
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
	nn = kheap_alloc(sizeof(sNode));
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

void sll_removeFirst(sSLList *list,void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;
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

	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
}

void sll_removeIndex(sSLList *list,u32 index) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;
	/* walk to the desired position */
	while(index-- > 0) {
		ln = n;
		n = n->next;
	}

	if(n == NULL)
		panic("Index %d does not exist!",index);

	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
}

static sNode *sll_gesNode(sSLList *list,u32 index) {
	sList *l = (sList*)list;
	sNode *n;
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
