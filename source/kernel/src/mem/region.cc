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
#include <sys/mem/physmem.h>
#include <sys/mem/cache.h>
#include <sys/mem/region.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/spinlock.h>
#include <sys/video.h>
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

Region *Region::create(sFile *file,size_t bCount,size_t lCount,size_t offset,ulong pgFlags,ulong flags) {
	size_t i,pageCount;
	Region *reg;
	/* a region can never be shareable AND growable. this way, we don't need to lock every region-
	 * access. because either its growable, then there is only one process that uses the region,
	 * whose region-stuff is locked anyway. or its shareable, then byteCount, pfSize and pageFlags
	 * can't change. */
	assert((flags & (RF_SHAREABLE | RF_GROWABLE)) != (RF_SHAREABLE | RF_GROWABLE));
	assert(pgFlags == (ulong)-1 || pgFlags == PF_DEMANDLOAD || pgFlags == 0);

	reg = (Region*)Cache::alloc(sizeof(Region));
	if(reg == NULL)
		return NULL;
	sll_init(&reg->vms,slln_allocNode,slln_freeNode);
	if(file) {
		reg->offset = offset;
		reg->file = file;
		reg->loadCount = lCount;
	}
	else {
		reg->file = NULL;
		reg->offset = 0;
		reg->loadCount = 0;
	}
	reg->flags = flags;
	reg->byteCount = bCount;
	reg->timestamp = 0;
	reg->lock = 0;
	pageCount = BYTES_2_PAGES(bCount);
	/* if we have no pages, create the page-array with 1; using 0 will fail and this may actually
	 * happen for data-regions of zero-size. We want to be able to increase their size, so they
	 * must exist. */
	reg->pfSize = MAX(1,pageCount);
	reg->pageFlags = (ulong*)Cache::alloc(reg->pfSize * sizeof(ulong));
	if(reg->pageFlags == NULL) {
		sll_clear(&reg->vms,false);
		Cache::free(reg);
		return NULL;
	}
	/* -1 means its initialized later (for reg_clone) */
	if(pgFlags != (ulong)-1) {
		for(i = 0; i < pageCount; i++)
			reg->pageFlags[i] = pgFlags;
	}
	return reg;
}

Region *Region::clone(const void *p) const {
	Region *c;
	assert(!(flags & RF_SHAREABLE));
	c = create(file,byteCount,loadCount,offset,-1,flags);
	if(c) {
		/* increment references to swap-blocks */
		size_t i,count = BYTES_2_PAGES(byteCount);
		for(i = 0; i < count; i++) {
			c->pageFlags[i] = pageFlags[i];
			if(pageFlags[i] & PF_SWAPPED)
				SwapMap::incRefs(getSwapBlock(i));
		}
		c->addTo(p);
	}
	return c;
}

void Region::destroy() {
	size_t i,pcount = BYTES_2_PAGES(byteCount);
	/* first free the swapped out blocks */
	for(i = 0; i < pcount; i++) {
		if(pageFlags[i] & PF_SWAPPED)
			SwapMap::free(getSwapBlock(i));
	}
	Cache::free(pageFlags);
	sll_clear(&vms,false);
	Cache::free(this);
}

size_t Region::pageCount(size_t *swapped) const {
	size_t i,c = 0,pcount = BYTES_2_PAGES(byteCount);
	assert(this != NULL);
	*swapped = 0;
	for(i = 0; i < pcount; i++) {
		if((pageFlags[i] & (PF_DEMANDLOAD | PF_SWAPPED)) == 0)
			c++;
		if(pageFlags[i] & PF_SWAPPED)
			(*swapped)++;
	}
	return c;
}

ssize_t Region::grow(ssize_t amount) {
	size_t count = BYTES_2_PAGES(byteCount);
	ssize_t res = 0;
	assert((flags & RF_GROWABLE));
	if(amount > 0) {
		ssize_t i;
		ulong *pf = (ulong*)Cache::realloc(pageFlags,(pfSize + amount) * sizeof(ulong));
		if(pf == NULL)
			return -ENOMEM;
		pfSize += amount;
		if(flags & RF_GROWS_DOWN) {
			memmove(pf + amount,pf,count * sizeof(ulong));
			for(i = 0; i < amount; i++)
				pf[i] = 0;
		}
		else {
			for(i = 0; i < amount; i++)
				pf[i + count] = 0;
		}
		pageFlags = pf;
		/* round up; if we add a page, we'll always have a complete page before that */
		byteCount = ROUND_PAGE_UP(byteCount);
		byteCount += amount * PAGE_SIZE;
	}
	else {
		size_t i;
		if(byteCount < (size_t)-amount * PAGE_SIZE)
			return -ENOMEM;
		/* free swapped pages */
		for(i = count + amount; i < count; i++) {
			if(pageFlags[i] & PF_SWAPPED) {
				SwapMap::free(getSwapBlock(i));
				res++;
			}
		}
		if(flags & RF_GROWS_DOWN)
			memmove(pageFlags,pageFlags + -amount,(count + amount) * sizeof(ulong));
		/* round up for the same reason */
		byteCount = ROUND_PAGE_UP(byteCount);
		byteCount -= -amount * PAGE_SIZE;
	}
	return res;
}

void Region::sprintf(sStringBuffer *buf,uintptr_t virt) const {
	size_t i,x;
	sSLNode *n;
	prf_sprintf(buf,"\tSize: %zu bytes\n",byteCount);
	prf_sprintf(buf,"\tLoad: %zu bytes\n",loadCount);
	prf_sprintf(buf,"\tflags: ");
	sprintfFlags(buf);
	prf_sprintf(buf,"\n");
	if(file) {
		prf_pushIndent();
		vfs_printFile(file);
		prf_popIndent();
	}
	prf_sprintf(buf,"\tTimestamp: %d\n",timestamp);
	prf_sprintf(buf,"\tProcesses: ");
	for(n = sll_begin(&vms); n != NULL; n = n->next)
		prf_sprintf(buf,"%d ",((VirtMem*)n->data)->getPid());
	prf_sprintf(buf,"\n");
	prf_sprintf(buf,"\tPages (%d):\n",BYTES_2_PAGES(byteCount));
	for(i = 0, x = BYTES_2_PAGES(byteCount); i < x; i++) {
		prf_sprintf(buf,"\t\t%d: (%p) (swblk %d) %c%c%c\n",i,virt + i * PAGE_SIZE,
				(pageFlags[i] & PF_SWAPPED) ? getSwapBlock(i) : 0,
				(pageFlags[i] & PF_COPYONWRITE) ? 'c' : '-',
				(pageFlags[i] & PF_DEMANDLOAD) ? 'l' : '-',
				(pageFlags[i] & PF_SWAPPED) ? 's' : '-');
	}
}

void Region::printFlags() const {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	sprintfFlags(&buf);
	if(buf.str) {
		Video::printf("%s",buf.str);
		Cache::free(buf.str);
	}
}

void Region::print(uintptr_t virt) const {
	sStringBuffer buf;
	buf.dynamic = true;
	buf.len = 0;
	buf.size = 0;
	buf.str = NULL;
	sprintf(&buf,virt);
	if(buf.str != NULL)
		Video::printf("%s",buf.str);
	else
		Video::printf("- no regions -\n");
	Cache::free(buf.str);
}

void Region::sprintfFlags(sStringBuffer *buf) const {
	struct {
		const char *name;
		ulong no;
	} flagNames[] = {
		{"Gr",RF_GROWABLE},
		{"Sh",RF_SHAREABLE},
		{"Wr",RF_WRITABLE},
		{"Ex",RF_EXECUTABLE},
		{"St",RF_STACK},
		{"NoFree",RF_NOFREE},
		{"TLS",RF_TLS},
		{"GrDwn",RF_GROWS_DOWN}
	};
	size_t i;
	for(i = 0; i < ARRAY_SIZE(flagNames); i++) {
		if(flags & flagNames[i].no)
			prf_sprintf(buf,"%s ",flagNames[i].name);
	}
}
