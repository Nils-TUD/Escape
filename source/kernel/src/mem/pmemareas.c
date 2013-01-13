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
#include <sys/mem/pmemareas.h>
#include <sys/mem/paging.h>
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>

#define MAX_PHYSMEM_AREAS			64

static sPhysMemArea *pmemareas_allocArea(void);

/* areas of (initially) free physical memory */
static sPhysMemArea areas[MAX_PHYSMEM_AREAS];
static size_t count = 0;
static sPhysMemArea *list = NULL;
static sPhysMemArea *listend = NULL;

frameno_t pmemareas_alloc(size_t frames) {
	sPhysMemArea *area = list;
	while(area != NULL) {
		if(area->size >= frames * PAGE_SIZE) {
			uintptr_t begin = area->addr;
			pmemareas_rem(begin,begin + frames * PAGE_SIZE);
			return begin / PAGE_SIZE;
		}
		area = area->next;
	}
	return 0;
}

sPhysMemArea *pmemareas_get(void) {
	return list;
}

size_t pmemareas_getAvailable(void) {
	sPhysMemArea *area = list;
	size_t total = 0;
	while(area != NULL) {
		total += area->size;
		area = area->next;
	}
	return total;
}

void pmemareas_add(uintptr_t addr,uintptr_t end) {
	sPhysMemArea *area = pmemareas_allocArea();
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

void pmemareas_rem(uintptr_t addr,uintptr_t end) {
	sPhysMemArea *prev = NULL;
	sPhysMemArea *area = list;
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
			pmemareas_add(end,area->addr + area->size);
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

void pmemareas_print(void) {
	sPhysMemArea *area = list;
	vid_printf("Free physical memory areas [in total %zu KiB]:\n",pmemareas_getAvailable() / K);
	while(area != NULL) {
		vid_printf("	%p .. %p [%zu KiB]\n",area->addr,area->addr + area->size - 1,
				area->size / K);
		area = area->next;
	}
}

static sPhysMemArea *pmemareas_allocArea(void) {
	if(count == MAX_PHYSMEM_AREAS)
		util_panic("Ran out of physmem-areas");
	return areas + count++;
}
