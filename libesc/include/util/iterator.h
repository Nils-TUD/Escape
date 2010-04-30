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

#ifndef ITERATOR_H_
#define ITERATOR_H_

#include <esc/common.h>

typedef struct sIterator sIterator;

typedef void *(*fItNext)(sIterator *it);
typedef bool (*fItHasNext)(sIterator *it);

/* All elements are READ-ONLY for the public! */
struct sIterator {
	void *con;
	u32 pos;
	fItNext next;
	fItHasNext hasNext;
};

#define foreach(v,type,eName)	\
	sIterator __it##v = vec_iterator(v); \
	type eName; \
	while((__it##v).hasNext(&__it##v) && \
			(eName = *(type*)(__it##v).next(&(__it##v))))

#endif /* ITERATOR_H_ */
