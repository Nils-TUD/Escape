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
#include <esc/debug.h>
#include <mem/heap.h>
#include <util/vector.h>
#include <string.h>
#include <assert.h>

#define INIT_SIZE		16

static int vec_sortCmp(const void *a,const void *b);
static void *vec_itNext(sIterator *it);
static bool vec_itHasNext(sIterator *it);
static void vec_grow(sVector *v,u32 reqSize);

static u32 sortElSize;

sVector *vec_create(u32 elSize) {
	return vec_createSize(elSize,INIT_SIZE);
}

sVector *vec_createSize(u32 elSize,u32 count) {
	sVector *v = (sVector*)heap_alloc(sizeof(sVector));
	v->elSize = elSize;
	v->size = elSize * count;
	v->count = 0;
	v->elements = heap_alloc(elSize * count);
	return v;
}

sVector *vec_copy(const sVector *v) {
	sVector *cpy = vec_createSize(v->elSize,v->count);
	memcpy(cpy->elements,v->elements,v->elSize * v->count);
	cpy->count = v->count;
	return cpy;
}

sIterator vec_iterator(sVector *v) {
	sIterator it;
	it._con = v;
	it._pos = 0;
	it.next = vec_itNext;
	it.hasNext = vec_itHasNext;
	return it;
}

void vec_addInt(sVector *v,u32 val) {
	assert(v->elSize <= sizeof(u32));
	vec_add(v,&val);
}

void vec_add(sVector *v,const void *p) {
	vec_grow(v,v->elSize * (v->count + 1));
	memcpy((char*)v->elements + v->count * v->elSize,p,v->elSize);
	v->count++;
}

void vec_insertInt(sVector *v,u32 index,u32 val) {
	assert(v->elSize <= sizeof(u32));
	vec_insert(v,index,&val);
}

void vec_insert(sVector *v,u32 index,const void *p) {
	char *dst = (char*)v->elements + v->elSize * index;
	assert(index <= v->count);
	vec_grow(v,v->elSize * (v->count + 1));
	if(index < v->count)
		memmove(dst + v->elSize,dst,(v->count - index) * v->elSize);
	memcpy(dst,p,v->elSize);
	v->count++;
}

void vec_removeInt(sVector *v,u32 val) {
	assert(v->elSize <= sizeof(u32));
	vec_removeObj(v,&val);
}

void vec_removeObj(sVector *v,const void *p) {
	s32 index = vec_find(v,p);
	if(index == -1)
		return;
	vec_remove(v,index,1);
}

void vec_remove(sVector *v,u32 start,u32 count) {
	assert(start < v->count);
	assert(start + count <= v->count && start + count >= start);
	if(start + count < v->count) {
		memmove((char*)v->elements + start * v->elSize,
				(char*)v->elements + (start + count) * v->elSize,
				(v->count - (start + count)) * v->elSize);
	}
	v->count -= count;
}

s32 vec_findInt(sVector *v,u32 val) {
	assert(v->elSize <= sizeof(u32));
	return vec_find(v,&val);
}

s32 vec_find(sVector *v,const void *p) {
	s32 i,count = v->count;
	for(i = 0; i < count; i++) {
		void *el = (char*)v->elements + i * v->elSize;
		if(memcmp(p,el,v->elSize) == 0)
			return i;
	}
	return -1;
}

void vec_sort(sVector *v) {
	sortElSize = v->elSize;
	qsort(v->elements,v->count,v->elSize,vec_sortCmp);
}

void vec_sortCustom(sVector *v,fCompare cmp) {
	qsort(v->elements,v->count,v->elSize,cmp);
}

void vec_destroy(sVector *v,bool freeElements) {
	if(freeElements) {
		sIterator it = vec_iterator(v);
		while(it.hasNext(&it)) {
			void *p = *(void**)it.next(&it);
			heap_free(p);
		}
	}
	heap_free(v->elements);
	heap_free(v);
}

static int vec_sortCmp(const void *a,const void *b) {
	return memcmp(a,b,sortElSize);
}

static void *vec_itNext(sIterator *it) {
	sVector *v = (sVector*)it->_con;
	if(it->_pos >= v->count)
		return NULL;
	return (char*)v->elements + it->_pos++ * v->elSize;
}

static bool vec_itHasNext(sIterator *it) {
	sVector *v = (sVector*)it->_con;
	return it->_pos < v->count;
}

static void vec_grow(sVector *v,u32 reqSize) {
	if(v->size < reqSize) {
		v->size = MAX(v->size * 2,reqSize);
		v->elements = heap_realloc(v->elements,v->size);
	}
}

#if DEBUGGING

void vec_dbg_print(sVector *v) {
	u32 i;
	debugf("Vector @ %x (%d bytes, %d elements, %d bytes each)\n",v,v->size,v->count,v->elSize);
	for(i = 0; i < v->count; i++) {
		u32 j;
		debugf("	");
		for(j = 0; j < v->elSize / sizeof(u32); j++)
			debugf("%08x",*(u32*)((char*)v->elements + v->elSize * i + j * sizeof(u32)));
		debugf("\n");
	}
	debugf("\n");
}

#endif
