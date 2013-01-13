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
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/boot.h>
#include <sys/log.h>

extern void *_ebss;
static size_t total;

void pmemareas_initArch(void) {
	size_t i;
	sMemMap *mmap;
	sModule *mod;
	const sBootInfo *mb = boot_getInfo();

	/* walk through the memory-map and mark all free areas as free */
	for(mmap = mb->mmapAddr; (uintptr_t)mmap < (uintptr_t)mb->mmapAddr + mb->mmapLength;
			mmap = (sMemMap*)((uintptr_t)mmap + mmap->size + sizeof(mmap->size))) {
		if(mmap != NULL && mmap->type == MMAP_TYPE_AVAILABLE) {
			/* take care that we don't use memory above 4G */
			if(mmap->baseAddr >= 0x100000000ULL) {
				log_printf("Skipping memory above 4G: %#Lx .. %#Lx\n",
						mmap->baseAddr,mmap->baseAddr + mmap->length);
				continue;
			}
			uint64_t end = mmap->baseAddr + mmap->length;
			if(end >= 0x100000000ULL) {
				log_printf("Skipping memory above 4G: %#Lx .. %#Lx\n",0x100000000ULL,end);
				end = 0xFFFFFFFF;
			}
			pmemareas_add((uintptr_t)mmap->baseAddr,(uintptr_t)end);
		}
	}
	total = pmemareas_getAvailable();

	/* remove kernel and the first MB */
	pmemareas_rem(0,(uintptr_t)&_ebss - KERNEL_AREA);

	/* remove modules */
	mod = mb->modsAddr;
	for(i = 0; i < mb->modsCount; i++) {
		pmemareas_rem(mod->modStart,mod->modEnd);
		mod++;
	}
}

size_t pmemareas_getTotal(void) {
	return total;
}
