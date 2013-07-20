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

#include <sys/common.h>
#include <sys/mem/vmfree.h>
#include <sys/mem/cache.h>
#include <sys/video.h>
#include <assert.h>

/**
 * The idea here is to allow a quick allocation and release of areas in the free-area of the
 * virtual address space (for shared mem, shared libraries, TLS, ...). Therefore, we use a
 * malloc-like management that keeps track of the free areas (not the used ones).
 */

bool vmfree_init(sVMFreeMap *map,uintptr_t addr,size_t size) {
	sVMFreeArea *a = (sVMFreeArea*)Cache::alloc(sizeof(sVMFreeArea));
	if(!a)
		return false;
	a->addr = addr;
	a->size = size;
	a->next = NULL;
	map->list = a;
	return true;
}

void vmfree_destroy(sVMFreeMap *map) {
	sVMFreeArea *a;
	for(a = map->list; a != NULL; ) {
		sVMFreeArea *n = a->next;
		Cache::free(a);
		a = n;
	}
	map->list = NULL;
}

uintptr_t vmfree_allocate(sVMFreeMap *map,size_t size) {
	sVMFreeArea *a,*p;
	uintptr_t res;
	/* TODO is that correct on archs with page-size != 0x1000? */
	assert((size & 0xFFF) == 0);
	p = NULL;
	for(a = map->list; a != NULL; p = a, a = a->next) {
		if(a->size >= size)
			break;
	}
	if(a == NULL)
		return 0;

	/* take it from the front */
	res = a->addr;
	a->size -= size;
	a->addr += size;
	/* if the area is empty now, remove it */
	if(a->size == 0) {
		if(p)
			p->next = a->next;
		else
			map->list = a->next;
		Cache::free(a);
	}
	return res;
}

bool vmfree_allocateAt(sVMFreeMap *map,uintptr_t addr,size_t size) {
	sVMFreeArea *a,*p;
	p = NULL;
	for(a = map->list; a != NULL && addr > a->addr + a->size; p = a, a = a->next)
		;
	/* invalid position or too small? */
	if(a == NULL || addr + size > a->addr + a->size || (!p && addr < a->addr))
		return false;

	/* complete area? */
	if(addr == a->addr && size == a->size) {
		if(p)
			p->next = a->next;
		else
			map->list = a->next;
		Cache::free(a);
	}
	/* at the beginning? */
	else if(addr == a->addr) {
		a->addr += size;
		a->size -= size;
	}
	/* at the end? */
	else if(addr + size == a->addr + a->size) {
		a->size -= size;
	}
	/* in the middle */
	else {
		sVMFreeArea *na = (sVMFreeArea*)Cache::alloc(sizeof(sVMFreeArea));
		if(!na)
			return false;
		na->addr = a->addr;
		na->size = addr - a->addr;
		na->next = a;
		if(p)
			p->next = na;
		else
			map->list = na;
		a->addr = addr + size;
		a->size -= na->size + size;
	}
	return true;
}

void vmfree_free(sVMFreeMap *map,uintptr_t addr,size_t size) {
	sVMFreeArea *n,*p;
	assert((size & 0xFFF) == 0);
	/* find the area behind ours */
	p = NULL;
	for(n = map->list; n != NULL && addr > n->addr; p = n, n = n->next)
		;

	/* merge with prev and next */
	if(p && p->addr + p->size == addr && n && addr + size == n->addr) {
		p->size += size + n->size;
		p->next = n->next;
		Cache::free(n);
	}
	/* merge with prev */
	else if(p && p->addr + p->size == addr) {
		p->size += size;
	}
	/* merge with next */
	else if(n && addr + size == n->addr) {
		n->addr -= size;
		n->size += size;
	}
	/* create new area between them */
	else {
		sVMFreeArea *a = (sVMFreeArea*)Cache::alloc(sizeof(sVMFreeArea));
		/* if this fails, ignore it; we can't really do something about it */
		if(!a)
			return;
		a->addr = addr;
		a->size = size;
		if(p)
			p->next = a;
		else
			map->list = a;
		a->next = n;
	}
}

size_t vmfree_getSize(sVMFreeMap *map,size_t *areas) {
	sVMFreeArea *a;
	size_t total = 0;
	*areas = 0;
	for(a = map->list; a != NULL; a = a->next) {
		total += a->size;
		(*areas)++;
	}
	return total;
}

void vmfree_print(sVMFreeMap *map) {
	sVMFreeArea *a;
	size_t areas;
	vid_printf("Free area with %zu bytes:\n",vmfree_getSize(map,&areas));
	for(a = map->list; a != NULL; a = a->next) {
		vid_printf("\t@ %p, %zu bytes\n",a->addr,a->size);
	}
}
