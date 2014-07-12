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

#include <common.h>
#include <mem/physmem.h>
#include <mem/cache.h>
#include <mem/region.h>
#include <mem/swapmap.h>
#include <task/proc.h>
#include <vfs/vfs.h>
#include <vfs/openfile.h>
#include <spinlock.h>
#include <video.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

/**
 * The region-module implements the abstraction 'region' which is simply a group of pages that
 * have common properties. So for example 'text' is a group, 'read-only-data', 'data', 'stack'
 * and so on. A region has no address and can be shared (if RF_SHAREABLE is set) by multiple
 * processes (and of course the virtual address to which it is mapped may be different).
 * A region has some flags that specify what operations are allowed and it has an array of
 * page-flags (flags for each page). So that each page can have different flags like copy-on-write,
 * swapped, demand-load or demand-zero. Additionally the page-flags store the swap-block if a
 * page is swapped out.
 */

/* TODO perhaps we should count how often a page was swapped out and in? based on this, we could
 * make a better decision for finding a victim */
/* TODO or should we count how often a region has been used instead/additionally? */

/* TODO actually, we don't need the page-flags, because every architecture should have enough space
 * in the page-table-entries. That is, for 3/4 flags and the swap-block, if the page is not in
 * memory. */

Region::Region(OpenFile *f,size_t bCount,size_t lCount,size_t off,ulong pgFlags,
               ulong _flags,bool &success)
		: flags(_flags), file(f), offset(off), loadCount(lCount), byteCount(bCount),
		  timestamp(0), pfSize(), pageFlags(), vms(), lock() {
	init(pgFlags,success);
}

Region::Region(const Region &reg,VirtMem *vm,bool &success)
		: flags(reg.flags), file(reg.file), offset(reg.offset), loadCount(reg.loadCount),
		  byteCount(reg.byteCount), timestamp(0), pfSize(), pageFlags(), vms(), lock() {
	assert(!(flags & RF_SHAREABLE));
	init(-1,success);
	if(!success)
		return;

	/* increment references to swap-blocks */
	size_t count = BYTES_2_PAGES(reg.byteCount);
	for(size_t i = 0; i < count; i++) {
		pageFlags[i] = reg.pageFlags[i];
		if(reg.pageFlags[i] & PF_SWAPPED)
			SwapMap::incRefs(reg.getSwapBlock(i));
	}
	addTo(vm);
}

void Region::init(ulong pgFlags,bool &success) {
	/* a region can never be shareable AND growable. this way, we don't need to lock every region-
	 * access. because either its growable, then there is only one process that uses the region,
	 * whose region-stuff is locked anyway. or its shareable, then byteCount, pfSize and pageFlags
	 * can't change. */
	assert((flags & (RF_SHAREABLE | RF_GROWABLE)) != (RF_SHAREABLE | RF_GROWABLE));
	assert(pgFlags == (ulong)-1 || pgFlags == PF_DEMANDLOAD || pgFlags == 0);

	if(!file) {
		offset = 0;
		loadCount = 0;
	}

	size_t pages = BYTES_2_PAGES(byteCount);
	/* if we have no pages, create the page-array with 1; using 0 will fail and this may actually
	 * happen for data-regions of zero-size. We want to be able to increase their size, so they
	 * must exist. */
	pfSize = MAX(1,pages);
	pageFlags = (ulong*)Cache::alloc(pfSize * sizeof(ulong));
	if(pageFlags == NULL) {
		/* make sure that we don't touch pageFlags in the destructor */
		byteCount = 0;
		success = false;
		return;
	}
	/* -1 means its initialized later (for reg_clone) */
	if(pgFlags != (ulong)-1) {
		for(size_t i = 0; i < pages; i++)
			pageFlags[i] = pgFlags;
	}
}

Region::~Region() {
	size_t pcount = BYTES_2_PAGES(byteCount);
	/* first free the swapped out blocks */
	for(size_t i = 0; i < pcount; i++) {
		if(pageFlags[i] & PF_SWAPPED)
			SwapMap::free(getSwapBlock(i));
	}
	Cache::free(pageFlags);
}

size_t Region::pageCount(size_t *swapped) const {
	size_t c = 0,pcount = BYTES_2_PAGES(byteCount);
	assert(this != NULL);
	*swapped = 0;
	for(size_t i = 0; i < pcount; i++) {
		if((pageFlags[i] & (PF_DEMANDLOAD | PF_SWAPPED)) == 0)
			c++;
		if(pageFlags[i] & PF_SWAPPED)
			(*swapped)++;
	}
	return c;
}

ssize_t Region::grow(ssize_t amount,size_t *own) {
	size_t count = BYTES_2_PAGES(byteCount);
	ssize_t res = 0;
	assert((flags & RF_GROWABLE));
	if(amount > 0) {
		ulong *pf = (ulong*)Cache::realloc(pageFlags,(pfSize + amount) * sizeof(ulong));
		if(pf == NULL)
			return -ENOMEM;
		pfSize += amount;
		if(flags & RF_GROWS_DOWN) {
			memmove(pf + amount,pf,count * sizeof(ulong));
			for(ssize_t i = 0; i < amount; i++)
				pf[i] = 0;
		}
		else {
			for(ssize_t i = 0; i < amount; i++)
				pf[i + count] = 0;
		}
		pageFlags = pf;
		/* round up; if we add a page, we'll always have a complete page before that */
		byteCount = ROUND_PAGE_UP(byteCount);
		byteCount += amount * PAGE_SIZE;
	}
	else {
		if(own)
			*own = 0;
		if(byteCount < (size_t)-amount * PAGE_SIZE)
			return -ENOMEM;
		/* free swapped pages */
		for(size_t i = count + amount; i < count; i++) {
			if(pageFlags[i] & PF_SWAPPED) {
				SwapMap::free(getSwapBlock(i));
				res++;
			}
			else if(own && (pageFlags[i] & (PF_COPYONWRITE | PF_DEMANDLOAD)) == 0)
				(*own)++;
		}
		if(flags & RF_GROWS_DOWN)
			memmove(pageFlags,pageFlags + -amount,(count + amount) * sizeof(ulong));
		/* round up for the same reason */
		byteCount = ROUND_PAGE_UP(byteCount);
		byteCount -= -amount * PAGE_SIZE;
	}
	return res;
}

void Region::print(OStream &os,uintptr_t virt) const {
	os.writef("\tSize: %zu bytes\n",byteCount);
	os.writef("\tLoad: %zu bytes\n",loadCount);
	os.writef("\tflags: ");
	printFlags(os);
	os.writef("\n");
	if(file) {
		os.writef("\tFile: ");
		file->print(os);
		os.writef("\n");
	}
	os.writef("\tTimestamp: %Lu\n",timestamp);
	os.writef("\tProcesses: ");
	for(auto it = vms.cbegin(); it != vms.cend(); ++it)
		os.writef("%d ",(*it)->getProc()->getPid());
	os.writef("\n");
	os.writef("\tPages (%d):\n",BYTES_2_PAGES(byteCount));
	for(size_t i = 0, x = BYTES_2_PAGES(byteCount); i < x; i++) {
		os.writef("\t\t%d: (%p) (swblk %d) %c%c%c\n",i,virt + i * PAGE_SIZE,
				(pageFlags[i] & PF_SWAPPED) ? getSwapBlock(i) : 0,
				(pageFlags[i] & PF_COPYONWRITE) ? 'c' : '-',
				(pageFlags[i] & PF_DEMANDLOAD) ? 'l' : '-',
				(pageFlags[i] & PF_SWAPPED) ? 's' : '-');
	}
}

void Region::printFlags(OStream &os) const {
	os.writef("%c%c%c%c%c%c%c",
		(flags & RF_WRITABLE) ? 'W' : 'w',
		(flags & RF_EXECUTABLE) ? 'X' : 'x',
		(flags & RF_GROWABLE) ? 'G' : 'g',
		(flags & RF_SHAREABLE) ? 'S' : 's',
		(flags & RF_LOCKED) ? 'L' : 'l',
		(flags & RF_GROWS_DOWN) ? 'D' : 'd',
		(flags & RF_NOFREE) ? 'f' : 'F');
}
