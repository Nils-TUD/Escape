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

#include <esc/common.h>
#include <esc/hashmap.h>
#include <assert.h>

#ifdef IN_KERNEL
#	include <sys/mem/cache.h>
#	include <sys/video.h>
/* for util_panic (vassert) */
#	include <sys/util.h>
#	define hmprintf		vid_printf
#	define hmfree		cache_free
#	define hmmalloc		cache_alloc
#else
/* for exit (vassert) */
#	include <esc/proc.h>
#	include <stdio.h>
#	include <stdlib.h>
#	define hmprintf		printf
#	define hmfree		free
#	define hmmalloc		malloc
#endif

sHashMap *hm_create(sSLList **map,size_t mapSize,fGetKey keyFunc) {
	sHashMap *m = (sHashMap*)hmmalloc(sizeof(sSLList*) * mapSize);
	if(!m)
		return NULL;
	m->map = map;
	m->mapSize = mapSize;
	m->elCount = 0;
	m->keyFunc = keyFunc;
	return m;
}

size_t hm_getCount(sHashMap *m) {
	return m->elCount;
}

void *hm_get(sHashMap *m,ulong key) {
	sSLNode *n;
	sSLList *l = m->map[key % m->mapSize];
	if(!l || m->elCount == 0)
		return NULL;
	for(n = sll_begin(l); n != NULL; n = n->next) {
		if(m->keyFunc(n->data) == key)
			return n->data;
	}
	return NULL;
}

bool hm_add(sHashMap *m,const void *data) {
	size_t index = m->keyFunc(data) % m->mapSize;
	sSLList *list = m->map[index];
	if(!list) {
		list = m->map[index] = sll_create();
		if(!list)
			return false;
	}
	if(sll_append(list,data)) {
		m->elCount++;
		return true;
	}
	return false;
}

void hm_remove(sHashMap *m,const void *data) {
	sSLNode *n,*p;
	ulong key = m->keyFunc(data);
	sSLList *l = m->map[key % m->mapSize];
	if(!l || m->elCount == 0)
		return;
	p = NULL;
	for(n = sll_begin(l); n != NULL; p = n,n = n->next) {
		if(m->keyFunc(n->data) == key) {
			sll_removeNode(l,n,p);
			if(sll_length(l) == 0) {
				sll_destroy(l,false);
				m->map[key % m->mapSize] = NULL;
			}
			m->elCount--;
			break;
		}
	}
}

void hm_begin(sHashMap *m) {
	m->curIndex = 0;
	m->curNode = NULL;
	m->curElIndex = 0;
}

void *hm_next(sHashMap *m) {
	sSLNode *n;
	if(m->curElIndex >= m->elCount)
		return NULL;
	if(m->curNode == NULL) {
		size_t i,size = m->mapSize;
		for(i = m->curIndex; i < size; i++) {
			sSLList *list = m->map[i];
			if(!list)
				continue;
			n = sll_begin(list);
			m->curNode = n->next;
			m->curIndex = i + 1;
			m->curElIndex++;
			return n->data;
		}
		return NULL;
	}
	n = m->curNode;
	m->curNode = m->curNode->next;
	m->curElIndex++;
	return n->data;
}

void hm_destroy(sHashMap *m) {
	size_t i;
	for(i = 0; i < m->mapSize; i++) {
		if(m->map[i])
			sll_destroy(m->map[i],false);
	}
	hmfree(m);
}

#if DEBUGGING

void hm_dbg_print(sHashMap *m) {
	size_t i;
	hmprintf("HashMap: elCount=%u, mapSize=%u\n",m->elCount,m->mapSize);
	for(i = 0; i < m->mapSize; i++) {
		hmprintf("\t%d: %x (%d elements)\n",i,m->map[i],m->map[i] ? sll_length(m->map[i]) : 0);
		if(m->map[i])
			sll_dbg_print(m->map[i]);
	}
}

#endif
