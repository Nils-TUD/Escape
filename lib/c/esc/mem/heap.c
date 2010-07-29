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
#include <esc/mem/heap.h>
#include <esc/exceptions/outofmemory.h>
#include <stdlib.h>

void *heap_alloc(u32 size) {
	void *p = malloc(size);
	if(size && !p)
		THROW(OutOfMemoryException);
	return p;
}

void *heap_realloc(void *p,u32 size) {
	p = realloc(p,size);
	if(size && !p)
		THROW(OutOfMemoryException);
	return p;
}

void *heap_calloc(u32 size) {
	void *p = calloc(size,1);
	if(size && !p)
		THROW(OutOfMemoryException);
	return p;
}

void heap_free(void *p) {
	free(p);
}
