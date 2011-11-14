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


#include <esc/common.h>
#include <esc/sllist.h>
#include <assert.h>

#ifdef IN_KERNEL
#	include <sys/mem/cache.h>
#	include <sys/mem/sllnodes.h>
#	include <sys/video.h>
/* for util_panic (vassert) */
#	include <sys/util.h>
#	define sllprintf	vid_printf
#	define heapalloc	cache_alloc
#	define heapfree		cache_free
#	define nodealloc	slln_allocNode
#	define nodefree		slln_freeNode
#else
/* for exit (vassert) */
#	include <esc/proc.h>
#	include <stdio.h>
#	include <stdlib.h>
#	define sllprintf	printf
#	define heapalloc	malloc
#	define heapfree		free
#	define nodealloc	malloc
#	define nodefree		free
#endif

/* a node in a list */
typedef struct sNode sNode;
struct sNode {
	sNode *next;
	const void *data;
};

/* represents a list */
typedef struct {
	fNodeAlloc falloc;
	fNodeFree ffree;
	sNode *first;
	sNode *last;
	size_t length;
} sList;

/**
 * Searches for the node at given index
 *
 * @param list the list
 * @param index the index
 * @return the node
 */
static sNode *sll_getNode(const sSLList *list,size_t index);

sSLList *sll_create(void) {
	sList *l = (sList*)heapalloc(sizeof(sList));
	if(l == NULL)
		return NULL;
	sll_init((sSLList*)l,(fNodeAlloc)nodealloc,(fNodeFree)nodefree);
	return (sSLList*)l;
}

sSLList *sll_createExtern(fNodeAlloc falloc,fNodeFree ffree) {
	sList *l = (sList*)heapalloc(sizeof(sList));
	if(l == NULL)
		return NULL;
	sll_init((sSLList*)l,falloc,ffree);
	return (sSLList*)l;
}

void sll_init(sSLList *list,fNodeAlloc falloc,fNodeFree ffree) {
	sList *l = (sList*)list;
	l->first = NULL;
	l->last = NULL;
	l->length = 0;
	l->falloc = falloc;
	l->ffree = ffree;
}

sSLList *sll_clone(const sSLList *list) {
	sSLNode *n;
	sSLList *l = sll_create();
	if(!l)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		if(!sll_append(l,n->data)) {
			sll_destroy(l,false);
			return NULL;
		}
	}
	return l;
}

void sll_destroy(sSLList *list,bool freeData) {
	if(list == NULL)
		return;
	sll_clear(list,freeData);
	heapfree(list);
}

void sll_clear(sSLList *list,bool freeData) {
	sList *l = (sList*)list;
	sNode *m,*n = (sNode*)l->first;
	vassert(list != NULL,"list == NULL");

	/* free all nodes */
	while(n != NULL) {
		m = n->next;
		if(freeData)
			heapfree((void*)n->data);
		l->ffree(n);
		n = m;
	}

	/* adjust list-properties */
	l->length = 0;
	l->first = NULL;
	l->last = NULL;
}

sSLNode *sll_begin(const sSLList *list) {
	return (sSLNode*)((sList*)list)->first;
}

sSLNode *sll_nodeAt(const sSLList *list,size_t index) {
	return (sSLNode*)sll_getNode(list,index);
}

size_t sll_length(const sSLList *list) {
	sList *l = (sList*)list;
	if(l == NULL)
		return 0;
	return l->length;
}

ssize_t sll_indexOf(const sSLList *list,const void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first;
	ssize_t i;
	for(i = 0; n != NULL; i++) {
		if(n->data == data)
			return i;
		n = n->next;
	}
	return -1;
}

sSLNode *sll_nodeWith(const sSLList *list,const void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first;
	while(n != NULL) {
		if(n->data == data)
			return (sSLNode*)n;
		n = n->next;
	}
	return NULL;
}

void *sll_get(const sSLList *list,size_t index) {
	sNode *n = sll_getNode(list,index);
	return n ? (void*)n->data : NULL;
}

void sll_set(sSLList *list,const void *data,size_t index) {
	sNode *n;
	n = sll_getNode(list,index);
	n->data = data;
}

bool sll_append(sSLList *list,const void *data) {
	sList *l = (sList*)list;
	return sll_insert(list,data,l->length);
}

bool sll_insert(sSLList *list,const void *data,size_t index) {
	sList *l = (sList*)list;
	sNode *ln = NULL;

	vassert(list != NULL,"list == NULL");

	/* walk to the desired position */
	if(index == l->length)
		ln = l->last;
	else {
		sNode *n = l->first;
		while(n != NULL && index-- > 0) {
			ln = n;
			n = n->next;
		}
	}

	return sll_insertAfter(list,(sSLNode*)ln,data);
}

bool sll_insertAfter(sSLList *list,sSLNode *prev,const void *data) {
	sList *l = (sList*)list;
	sNode *nn,*pn = (sNode*)prev;

	vassert(list != NULL,"list == NULL");

	/* allocate node? */
	nn = (sNode*)l->falloc(sizeof(sNode));
	if(nn == NULL)
		return false;

	/* insert */
	nn->data = data;
	if(pn != NULL) {
		nn->next = pn->next;
		pn->next = nn;
	}
	else {
		nn->next = l->first;
		l->first = nn;
	}
	if(nn->next == NULL)
		l->last = nn;
	l->length++;

	return true;
}

void sll_removeNode(sSLList *list,sSLNode *node,sSLNode *prev) {
	sList *l = (sList*)list;
	sNode *n = (sNode*)node,*ln = (sNode*)prev;

	vassert(list != NULL,"list == NULL");
	vassert(node != NULL,"node == NULL");
	vassert(ln == NULL || ln->next == n,"<prev> is not the previous node of <node>!");

	/* remove */
	if(ln != NULL)
		ln->next = n->next;
	else
		l->first = n->next;
	if(n->next == NULL)
		l->last = ln;
	l->length--;

	/* free */
	l->ffree(n);
}

void *sll_removeFirst(sSLList *list) {
	void *res;
	sList *l = (sList*)list;
	sNode *n;
	if(l->length == 0)
		return NULL;
	n = l->first;
	l->first = n->next;
	if(n->next == NULL)
		l->last = NULL;
	l->length--;
	res = (void*)n->data;
	l->ffree(n);
	return res;
}

ssize_t sll_removeFirstWith(sSLList *list,const void *data) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;
	ssize_t i = 0;

	vassert(list != NULL,"list == NULL");

	if(data != NULL) {
		while(n != NULL) {
			if(n->data == data)
				break;
			ln = n;
			n = n->next;
			i++;
		}
	}

	/* ignore */
	if(n == NULL)
		return -1;

	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
	return i;
}

void *sll_removeIndex(sSLList *list,size_t index) {
	sList *l = (sList*)list;
	sNode *n = l->first,*ln = NULL;
	void *res = NULL;

	vassert(list != NULL,"list == NULL");

	/* walk to the desired position */
	while(index-- > 0) {
		ln = n;
		n = n->next;
	}

	/* ignore */
	if(n == NULL)
		return NULL;

	res = (void*)n->data;
	sll_removeNode(list,(sSLNode*)n,(sSLNode*)ln);
	return res;
}

void sll_print(const sSLList *list) {
	sList *l = (sList*)list;
	if(l != NULL) {
		sNode *n = l->first;
		while(n != NULL) {
			sllprintf("\t[0x%x] data=0x%x, next=0x%x\n",n,n->data,n->next);
			n = n->next;
		}
	}
}

static sNode *sll_getNode(const sSLList *list,size_t index) {
	sList *l = (sList*)list;
	sNode *n;

	vassert(list != NULL,"list == NULL");
	vassert(index <= l->length,"The index %d does not exist",index);

	/* is it the last one? */
	if(index == l->length - 1)
		return l->last;

	/* walk to position */
	n = l->first;
	while(index-- > 0)
		n = n->next;
	return n;
}
