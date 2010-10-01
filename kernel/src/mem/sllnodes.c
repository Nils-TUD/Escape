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
		u32 i,oldCount;
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
