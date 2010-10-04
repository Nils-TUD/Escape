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

#include <sys/common.h>
#include <sys/mem/paging.h>
#include <sys/mem/dynarray.h>
#include <sys/mem/sllnodes.h>
#include <esc/sllist.h>
#include <assert.h>

/* The idea is to provide a very fast node-allocation and -deallocation for the single-linked-list
 * that is used in kernel. It is used at very many places, so that its really worth it.
 * To achieve that, we use the dynamic-array and dedicate an area in virtual-memory to the nodes.
 * This area is extended on demand and all nodes are put in a freelist (they have a next-element
 * anyway). This is much quicker than using the heap everytime a node is allocated or free'd.
 * That means, if we put no heap-allocated data in the sll (so data, that does exist anyway), using
 * it does basically cost nothing :) */

/* has to match the node of the sll */
typedef struct sNode sNode;
struct sNode {
	sNode *next;
	const void *data;
};

static bool initialized = false;
static sDynArray nodeArray;
static sNode *freelist = NULL;
static sNode *nodes = (sNode*)SLLNODE_AREA;

void *slln_allocNode(size_t size) {
	sNode *n;
	assert(sizeof(sNode) == size && offsetof(sSLNode,next) == offsetof(sNode,next));
	if(freelist == NULL) {
		size_t i,oldCount;
		if(!initialized) {
			dyna_init(&nodeArray,sizeof(sNode),SLLNODE_AREA,SLLNODE_AREA_SIZE);
			initialized = true;
		}
		oldCount = nodeArray.objCount;
		if(!dyna_extend(&nodeArray))
			return NULL;
		for(i = oldCount; i < nodeArray.objCount; i++) {
			nodes[i].next = freelist;
			freelist = nodes + i;
		}
	}
	n = freelist;
	freelist = freelist->next;
	return n;
}

void slln_freeNode(void *o) {
	sNode *n = (sNode*)o;
	n->next = freelist;
	freelist = n;
}
