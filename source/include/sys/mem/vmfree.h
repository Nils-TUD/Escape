/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#ifndef VMFREE_H_
#define VMFREE_H_

#include <sys/common.h>

typedef struct sVMFreeArea {
	uintptr_t addr;
	size_t size;
	struct sVMFreeArea *next;
} sVMFreeArea;

typedef struct {
	sVMFreeArea *list;
} sVMFreeMap;

bool vmfree_init(sVMFreeMap *map,uintptr_t addr,size_t size);
void vmfree_destroy(sVMFreeMap *map);
uintptr_t vmfree_allocate(sVMFreeMap *map,size_t size);
bool vmfree_allocateAt(sVMFreeMap *map,uintptr_t addr,size_t size);
void vmfree_free(sVMFreeMap *map,uintptr_t addr,size_t size);
size_t vmfree_getSize(sVMFreeMap *map,size_t *areas);
void vmfree_print(sVMFreeMap *map);

#endif /* VMFREE_H_ */
