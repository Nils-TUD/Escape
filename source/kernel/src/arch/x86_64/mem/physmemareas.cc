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

#include <sys/common.h>
#include <sys/mem/physmemareas.h>
#include <sys/mem/physmem.h>
#include <sys/mem/pagedir.h>
#include <sys/boot.h>
#include <sys/log.h>

extern void *_ebss;
static size_t total;

void PhysMemAreas::initArch() {
	const BootInfo *mb = Boot::getInfo();

	/* walk through the memory-map and mark all free areas as free */
	for(size_t i = 0; i < mb->mmapCount; ++i) {
		if(mb->mmap[i].type == MMAP_TYPE_AVAILABLE)
			PhysMemAreas::add((uintptr_t)mb->mmap[i].baseAddr,mb->mmap[i].baseAddr + mb->mmap[i].length);
	}
	total = PhysMemAreas::getAvailable();

	/* remove kernel and the first MB */
	PhysMemAreas::rem(0,(uintptr_t)&_ebss - KERNEL_BEGIN);

	/* remove modules */
	for(size_t i = 0; i < mb->modCount; i++)
		PhysMemAreas::rem(mb->mods[i].phys,mb->mods[i].phys + mb->mods[i].size);
}

size_t PhysMemAreas::getTotal() {
	return total;
}
