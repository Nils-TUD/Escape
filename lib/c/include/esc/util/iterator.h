/**
 * $Id: iterator.h 646 2010-05-04 15:19:36Z nasmussen $
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

struct sIterator {
/* private: */
	void *_con;
	u32 _pos;
	u32 _end;

/* public: */
	/**
	 * Checks wether there is a next element
	 *
	 * @param it the iterator
	 * @return true if so
	 */
	bool (*hasNext)(sIterator *it);

	/**
	 * Returns the next element and moves forward
	 *
	 * @param it the iterator
	 * @return the element
	 */
	void *(*next)(sIterator *it);
};

#define foreach(it,eName)	\
	sIterator __it##eName = (it); \
	while((__it##eName).hasNext(&__it##eName) && \
			(eName = (__typeof__(eName))(__it##eName).next(&(__it##eName))))

#endif /* ITERATOR_H_ */
