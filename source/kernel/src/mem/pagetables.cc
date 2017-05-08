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
#include <mem/pagetables.h>
#include <task/proc.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <ostream.h>
#include <util.h>

bool PageTables::hasNXE = false;

void PageTables::RangeAllocator::freePage(frameno_t) {
	Util::panic("Not supported");
}

PageTables::UAllocator::UAllocator() : Allocator(), _thread(Thread::getRunning()) {
}

frameno_t PageTables::UAllocator::allocPage() {
	return _thread->getFrame();
}

frameno_t PageTables::KAllocator::allocPT() {
	Util::panic("Trying to allocate a page-table in kernel-area");
	return 0;
}

void PageTables::KAllocator::freePT(frameno_t) {
	Util::panic("Trying to free a page-table in kernel-area");
}

int PageTables::clone(PageTables *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share) {
	NoAllocator noalloc;
	PageTables *cur = Proc::getCurPageDir()->getPageTables();
	uintptr_t base,orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	assert(this != dst && (this == cur || dst == cur));
	while(count > 0) {
		pte_t *spt = getPTE(virtSrc,&base);
		pte_t pte = *spt;

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_PRESENT)))
			pte &= ~PTE_WRITABLE;

		int res = dst->mapPage(virtDst,PTE_FRAMENO(pte),pte & ~PTE_FRAMENO_MASK,noalloc);
		if(res < 0)
			goto error;
		/* we never need a flush here because it was not present before */

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_PRESENT)) {
			uint flags = pte & ~(PTE_FRAMENO_MASK | PTE_WRITABLE);
			sassert(mapPage(virtSrc,PTE_FRAMENO(pte),flags,noalloc) >= 0);
			if(this == cur)
				flushAddr(virtSrc,true);
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	return noalloc.pageTables();

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	dst->unmap(orgVirtDst,orgCount - count,noalloc);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		pte_t *pte = getPTE(orgVirtSrc,&base);
		if(!share && (*pte & PTE_PRESENT))
			mapPage(orgVirtSrc,PTE_FRAMENO(*pte),PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS,noalloc);
		orgVirtSrc += PAGE_SIZE;
		orgCount--;
	}
	return -ENOMEM;
}

int PageTables::crtPageTable(pte_t *pte,uint flags,Allocator &alloc) {
	frameno_t frame = alloc.allocPT();
	if(frame == PhysMem::INVALID_FRAME)
		return -ENOMEM;

	assert((*pte & PTE_PRESENT) == 0);
	*pte = frame << PAGE_BITS | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	if(!(flags & PG_SUPERVISOR))
		*pte |= PTE_NOTSUPER;

	memclear((void*)(DIR_MAP_AREA + (frame << PAGE_BITS)),PAGE_SIZE);
	return 0;
}

int PageTables::mapPage(uintptr_t virt,frameno_t frame,pte_t flags,Allocator &alloc) {
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + root);
	uint bits = PT_BITS - PT_BPL;
	for(int i = 0; i < PT_LEVELS - 1; ++i) {
		uintptr_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
		if(pt[idx] == 0) {
			if(crtPageTable(pt + idx,flags,alloc) < 0)
				return -ENOMEM;
		}
		pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pt[idx] & PTE_FRAMENO_MASK));
		bits -= PT_BPL;
	}

	uintptr_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
	bool wasPresent = pt[idx] & PTE_PRESENT;
	if(frame)
		pt[idx] = (frame << PAGE_BITS) | flags;
	else
		pt[idx] = (pt[idx] & PTE_FRAMENO_MASK) | flags;
	return wasPresent;
}

pte_t *PageTables::getPTE(uintptr_t virt,uintptr_t *base) const {
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + root);
	uint bits = PT_BITS - PT_BPL;
	for(int i = 0; i < PT_LEVELS - 1; ++i) {
		uintptr_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
		if(~pt[idx] & PTE_PRESENT)
			return NULL;
		if(pt[idx] & PTE_LARGE) {
			*base = virt & ~((1 << bits) - 1);
			return pt + idx;
		}
		pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pt[idx] & PTE_FRAMENO_MASK));
		bits -= PT_BPL;
	}

	uintptr_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
	return pt + idx;
}

frameno_t PageTables::unmapPage(uintptr_t virt) {
	uintptr_t base = virt;
	pte_t *pte = getPTE(virt,&base);
	frameno_t frame = 0;
	if(pte) {
		assert(base == virt);
		frame = (*pte & PTE_PRESENT) ? PTE_FRAMENO(*pte) : 0;
		*pte = 0;
	}
	return frame;
}

bool PageTables::gc(uintptr_t virt,pte_t pte,int level,uint bits,Allocator &alloc) {
	if(~pte & PTE_EXISTS)
		return true;
	if(level == 0)
		return false;

	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pte & PTE_FRAMENO_MASK));
	size_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
	if(pt[idx] & PTE_PRESENT) {
		if(!gc(virt,pt[idx],level - 1,bits - PT_BPL,alloc))
			return false;

		/* free that page-table */
		alloc.freePT(PTE_FRAMENO(pt[idx]));
		pt[idx] = 0;
		flushPT(virt);
	}

	/* for not-top-level page-tables, check if all other entries are empty now as well */
	if(level < PT_LEVELS) {
		virt &= ~((1 << (bits + PT_BPL)) - 1);
		for(size_t i = 0; i < PT_ENTRY_COUNT; ++i) {
			if((pt[i] & PTE_EXISTS) && !gc(virt,pt[i],level - 1,bits - PT_BPL,alloc))
				return false;
			virt += 1 << bits;
		}
	}
	return true;
}

int PageTables::map(uintptr_t virt,size_t count,Allocator &alloc,uint flags) {
	PageTables *cur = Proc::getCurPageDir()->getPageTables();
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	pte_t pteFlags = 0;

	virt &= ~(PAGE_SIZE - 1);
	if(~flags & PG_NOPAGES)
		pteFlags = PTE_EXISTS;
	if(flags & PG_PRESENT)
		pteFlags |= PTE_PRESENT;
	if((flags & PG_PRESENT) && (flags & PG_WRITABLE))
		pteFlags |= PTE_WRITABLE;
	if(!(flags & PG_SUPERVISOR))
		pteFlags |= PTE_NOTSUPER;
	if(flags & PG_GLOBAL)
		pteFlags |= PTE_GLOBAL;
	if(hasNXE && (~flags & PG_EXECUTABLE))
		pteFlags |= PTE_NO_EXEC;

	bool needShootdown = false;
	while(count > 0) {
		frameno_t frame = 0;
		if(flags & PG_PRESENT) {
			frame = alloc.allocPage();
			if(frame == PhysMem::INVALID_FRAME)
				goto error;
		}

		int res = mapPage(virt,frame,pteFlags,alloc);
		if(res < 0)
			goto error;
		needShootdown |= res == 1;

		/* invalidate TLB-entry */
		if(this == cur)
			flushAddr(virt,res == 1);

		/* to next page */
		virt += PAGE_SIZE;
		count--;
	}
	return needShootdown;

error:
	unmap(orgVirt,orgCount - count,alloc);
	return -ENOMEM;
}

int PageTables::unmap(uintptr_t virt,size_t count,Allocator &alloc) {
	PageTables *cur = Proc::getCurPageDir()->getPageTables();
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	bool needShootdown = false;
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = index(virt,1);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && (virt < KERNEL_AREA || virt >= KSTACK_AREA))
				gc(virt - PAGE_SIZE,root | PTE_EXISTS,PT_LEVELS,PT_BITS - PT_BPL,alloc);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		frameno_t frame = unmapPage(virt);
		if(frame) {
			alloc.freePage(frame);
			/* invalidate TLB-entry */
			if(this == cur)
				flushAddr(virt,true);
			needShootdown = true;
		}

		/* to next page */
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && (virt < KERNEL_AREA || virt >= KSTACK_AREA))
		gc(virt - PAGE_SIZE,root | PTE_EXISTS,PT_LEVELS,PT_BITS - PT_BPL,alloc);
	return needShootdown;
}

size_t PageTables::countEntries(pte_t pte,int level) const {
	size_t count = 0;
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pte & PTE_FRAMENO_MASK));
	size_t end = level == PT_LEVELS ? index(KERNEL_AREA,level - 1) : PT_ENTRY_COUNT;
	for(size_t i = 0; i < end; ++i) {
		if(pt[i] & PTE_PRESENT) {
			if(level == 1)
				count++;
			else if(level > 1)
				count += countEntries(pt[i],level - 1);
		}
	}
	return count;
}

void PageTables::print(OStream &os,uint parts) const {
	if(parts & PD_PART_USER)
		printPTE(os,0,KERNEL_AREA,root,PT_LEVELS);
	if(parts & PD_PART_KERNEL)
		printPTE(os,KERNEL_AREA,(ulong)-1,root,PT_LEVELS);
}

void PageTables::printPTE(OStream &os,uintptr_t from,uintptr_t to,pte_t page,int level) {
	uint64_t size = (uint64_t)PAGE_SIZE << (PT_BPL * level);
	size_t entrySize = (size_t)PAGE_SIZE << (PT_BPL * (level - 1));
	uintptr_t base = from & ~(size - 1);
	size_t idx = (from >> (PAGE_BITS + PT_BPL * level)) & (PT_ENTRY_COUNT - 1);
	os.writef("%*s%03zx: %0*lx [%c%c%c%c%c%c] (%p..%p)\n",PT_LEVELS - level,"",
			idx,sizeof(page) * 2,page,
			(page & PTE_PRESENT) ? 'p' : '-',(page & PTE_NOTSUPER) ? 'u' : 'k',
			(page & PTE_WRITABLE) ? 'w' : 'r',(level > 0 || (page & PTE_NO_EXEC)) ? '-' : 'x',
			(page & PTE_GLOBAL) ? 'g' : '-',(page & PTE_LARGE) ? 'l' : '-',
			base,(uintptr_t)(base + size - 1));
	if(level > 0 && (page & PTE_LARGE) == 0) {
		pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (page & PTE_FRAMENO_MASK));
		size_t end,start = from >> (PAGE_BITS + PT_BPL * (level - 1)) & (PT_ENTRY_COUNT - 1);
		uintptr_t last = to + entrySize - 1;
		if(esc::Util::max(to,last) >= base + entrySize * PT_ENTRY_COUNT - 1)
			end = PT_ENTRY_COUNT;
		else
			end = last >> (PAGE_BITS + PT_BPL * (level - 1)) & (PT_ENTRY_COUNT - 1);
		for(; start < end; ++start) {
			if(pt[start] & PTE_EXISTS)
				printPTE(os,from,esc::Util::min(from + entrySize - 1,to),pt[start],level - 1);
			from += entrySize;
		}
	}
}
