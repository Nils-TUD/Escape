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

#ifndef VECTOR_H_
#define VECTOR_H_

#include <esc/common.h>
#include <esc/algo.h>
#include <util/iterator.h>

/* All elements are READ-ONLY for the public! */
typedef struct {
	void *elements;
	u32 size;			/* size of the memarea <elements> */
	u32 elSize;			/* size of one element */
	u32 count;			/* number of elements in the vector */
} sVector;

sVector *vec_create(u32 elSize);
sVector *vec_createSize(u32 elSize,u32 count);
sVector *vec_copy(const sVector *v);
sIterator vec_iterator(sVector *v);
void vec_addInt(sVector *v,u32 val);
void vec_add(sVector *v,const void *p);
void vec_insertInt(sVector *v,u32 index,u32 val);
void vec_insert(sVector *v,u32 index,const void *p);
void vec_removeInt(sVector *v,u32 val);
void vec_removeObj(sVector *v,const void *p);
void vec_remove(sVector *v,u32 start,u32 count);
s32 vec_findInt(sVector *v,u32 val);
s32 vec_find(sVector *v,const void *p);
void vec_sort(sVector *v);
void vec_sortCustom(sVector *v,fCompare cmp);
void vec_destroy(sVector *v,bool freeElements);

#if DEBUGGING
void vec_dbg_print(sVector *v);
#endif

#endif /* VECTOR_H_ */
