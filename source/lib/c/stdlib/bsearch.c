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
#include <stdlib.h>

void *bsearch(const void *key,const void *base,size_t num,size_t size,fCompare cmp) {
	size_t from = 0;
	size_t to = num - 1;
	size_t m;
	int res;
	void *val;

	while(from <= to) {
		m = (from + to) / 2;
		val = (void*)((size_t)base + m * size);

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
