/**
 * $Id: vector.c 716 2010-07-29 09:39:46Z nasmussen $
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../mem.h"
#include "vector.h"

#define INIT_SIZE		16

static void vec_grow(sVector *v,u32 reqSize);

sVector *vec_create(u32 elSize) {
	return vec_createSize(elSize,INIT_SIZE);
}

sVector *vec_createSize(u32 elSize,u32 count) {
	sVector *v = (sVector*)emalloc(sizeof(sVector));
	v->_elSize = elSize;
	v->_size = elSize * count;
	v->_elements = emalloc(elSize * count);
	v->count = 0;
	return v;
}

sVector *vec_copy(const sVector *v) {
	sVector *cpy = vec_createSize(v->_elSize,v->count);
	memcpy(cpy->_elements,v->_elements,v->_elSize * v->count);
	cpy->count = v->count;
	return cpy;
}

void *vec_get(sVector *v,u32 i) {
	assert(i < v->count);
	return *(void**)((char*)v->_elements + i * v->_elSize);
}

void vec_add(sVector *v,const void *p) {
	vec_grow(v,v->_elSize * (v->count + 1));
	memcpy((char*)v->_elements + v->count * v->_elSize,p,v->_elSize);
	v->count++;
}

void vec_set(sVector *v,u32 index,const void *p) {
	assert(index < v->count);
	memcpy((char*)v->_elements + index * v->_elSize,p,v->_elSize);
}

void vec_insert(sVector *v,u32 index,const void *p) {
	char *dst = (char*)v->_elements + v->_elSize * index;
	assert(index <= v->count);
	vec_grow(v,v->_elSize * (v->count + 1));
	if(index < v->count)
		memmove(dst + v->_elSize,dst,(v->count - index) * v->_elSize);
	memcpy(dst,p,v->_elSize);
	v->count++;
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
		memmove((char*)v->_elements + start * v->_elSize,
				(char*)v->_elements + (start + count) * v->_elSize,
				(v->count - (start + count)) * v->_elSize);
	}
	v->count -= count;
}

s32 vec_find(sVector *v,const void *p) {
	s32 i,count = v->count;
	for(i = 0; i < count; i++) {
		void *el = (char*)v->_elements + i * v->_elSize;
		if(memcmp(p,el,v->_elSize) == 0)
			return i;
	}
	return -1;
}

void vec_destroy(sVector *v,bool freeElements) {
	if(freeElements) {
		s32 i,count = v->count;
		for(i = 0; i < count; i++)
			efree(*(void**)((char*)v->_elements + i * v->_elSize));
	}
	efree(v->_elements);
	efree(v);
}

static void vec_grow(sVector *v,u32 reqSize) {
	if(v->_size < reqSize) {
		v->_size = MAX(v->_size * 2,reqSize);
		v->_elements = erealloc(v->_elements,v->_size);
	}
}

#if DEBUGGING

void vec_dbg_print(sVector *v) {
	u32 i;
	debugf("Vector @ %x (%d bytes, %d elements, %d bytes each)\n",v,v->_size,v->count,v->_elSize);
	for(i = 0; i < v->count; i++) {
		u32 j;
		debugf("	");
		for(j = 0; j < v->_elSize / sizeof(u32); j++)
			debugf("%08x",*(u32*)((char*)v->_elements + v->_elSize * i + j * sizeof(u32)));
		debugf("\n");
	}
	debugf("\n");
}

#endif
