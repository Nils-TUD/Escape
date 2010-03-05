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
#include <esc/algo.h>
#include <string.h>

/* macro to get a pointer to element <elNo> from <array> with a size of <elSize> per element */
#define QSORT_AGET(array,elSize,elNo) (((char*)(array)) + (elSize) * (elNo))

/**
 * Quicksort-implementation
 * source: http://de.wikipedia.org/wiki/Quicksort
 */
static void _qsort(void *base,size_t size,fCompare cmp,s32 left,s32 right);
static int divide(void *base,size_t size,fCompare cmp,s32 left,s32 right);

void *bsearch(const void *key,const void *base,u32 num,u32 size,fCompare cmp) {
	u32 from = 0;
	u32 to = num - 1;
	u32 m;
	s32 res;
	void *val;

	while(from <= to) {
		m = (from + to) / 2;
		val = (void*)((u32)base + m * size);

		/* compare */
		res = cmp(val,key);

		/* have we found it? */
		if(res == 0)
			return val;

		/* where to continue? */
		if(res > 0)
			to = m - 1;
		else
			from = m + 1;
	}
	return NULL;
}

void qsort(void *base,u32 nmemb,u32 size,fCompare cmp) {
	_qsort(base,size,cmp,0,nmemb - 1);
}

static void _qsort(void *base,u32 size,fCompare cmp,s32 left,s32 right) {
	/* TODO someday we should provide a better implementation which uses another sort-algo
	 * for small arrays, don't uses recursion and so on */
	if(left < right) {
		int i = divide(base,size,cmp,left,right);
		_qsort(base,size,cmp,left,i - 1);
		_qsort(base,size,cmp,i + 1,right);
	}
}

static int divide(void *base,u32 size,fCompare cmp,s32 left,s32 right) {
	char *pleft = (char*)base + left * size;
	char *piv = (char*)base + right * size;
	char *i = pleft;
	char *j = piv - size;
	do {
		/* right until the element is > piv */
		while(cmp(i,piv) <= 0 && i < piv)
			i += size;
		/* left until the element is < piv */
		while(cmp(j,piv) >= 0 && j > pleft)
			j -= size;

		/* swap */
		if(i < j) {
			memswp(i,j,size);
			i += size;
			j -= size;
		}
	}
	while(i < j);

	/* swap piv with element i */
	if(cmp(i,piv) > 0)
		memswp(i,piv,size);
	return (u32)(i - (char*)base) / size;
}
