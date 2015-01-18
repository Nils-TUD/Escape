/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <mem/pagedir.h>
#include <mem/physmemareas.h>
#include <assert.h>
#include <common.h>
#include <util.h>
#include <video.h>

/* areas of (initially) free physical memory */
PhysMemAreas::MemArea PhysMemAreas::areas[MAX_PHYSMEM_AREAS];
size_t PhysMemAreas::count = 0;
PhysMemAreas::MemArea *PhysMemAreas::list = NULL;
PhysMemAreas::MemArea *PhysMemAreas::listend = NULL;

frameno_t PhysMemAreas::alloc(size_t frames) {
	MemArea *area = list;
	while(area != NULL) {
		if(area->size >= frames * PAGE_SIZE) {
			uintptr_t begin = area->addr;
			rem(begin,begin + frames * PAGE_SIZE);
			return begin / PAGE_SIZE;
		}
		area = area->next;
	}
	return INVALID_FRAME;
}

size_t PhysMemAreas::getAvailable() {
	MemArea *area = list;
	size_t total = 0;
	while(area != NULL) {
		total += area->size;
		area = area->next;
	}
	return total;
}

void PhysMemAreas::add(uintptr_t addr,uintptr_t end) {
	MemArea *area = allocArea();
	addr = ROUND_PAGE_UP(addr);
	end = ROUND_PAGE_UP(end);
	area->addr = addr;
	area->size = end - addr;
	area->next = NULL;
	if(listend)
		listend->next = area;
	else
		list = area;
	listend = area;
}

void PhysMemAreas::rem(uintptr_t addr,uintptr_t end) {
	MemArea *prev = NULL;
	MemArea *area = list;
	addr = ROUND_PAGE_DN(addr);
	end = ROUND_PAGE_UP(end);
	while(area != NULL) {
		/* area completely covered -> remove */
		if(addr <= area->addr && end >= area->addr + area->size) {
			if(prev)
				prev->next = area->next;
			else
				list = area->next;
			if(area == listend)
				listend = prev;
			area = area->next;
			continue;
		}
		/* in the middle of the area */
		else if(addr > area->addr && end < area->addr + area->size) {
			add(end,area->addr + area->size);
			area->size = addr - area->addr;
		}
		/* beginning covered, i.e. end is within the area */
		else if(end > area->addr && end < area->addr + area->size) {
			area->size = (area->addr + area->size) - end;
			area->addr = end;
		}
		/* end covered, i.e. begin is within the area */
		else if(addr > area->addr && addr < area->addr + area->size)
			area->size = addr - area->addr;

		prev = area;
		area = area->next;
	}
}

void PhysMemAreas::print(OStream &os) {
	MemArea *area = list;
	os.writef("Free physical memory areas [in total %zu KiB]:\n",getAvailable() / 1024);
	while(area != NULL) {
		os.writef("	%p .. %p [%zu KiB]\n",area->addr,area->addr + area->size - 1,
				area->size / 1024);
		area = area->next;
	}
}

PhysMemAreas::MemArea *PhysMemAreas::allocArea() {
	if(count == MAX_PHYSMEM_AREAS)
		Util::panic("Ran out of physmem-areas");
	return areas + count++;
}
