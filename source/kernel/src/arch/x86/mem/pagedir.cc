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
#include <sys/mem/pagedir.h>
#include <sys/task/proc.h>
#include <sys/task/smp.h>
#include <sys/util.h>
#include <assert.h>

/**
 * Flushes the TLB-entry for the given virtual address.
 * Note: supported for >= Intel486
 */
#define	FLUSHADDR(addr)			asm volatile ("invlpg (%0)" : : "r" (addr));

extern void *proc0TLPD;
uintptr_t PageDir::freeAreaAddr = KFREE_AREA;

/* Note that we only need a lock for the temp-page here, because everything else is not shared
 * among different modules. First, the only critical state here are the page-tables. They are
 * created at boot for the kernel and in map(), join() or fork() for the user-side. For the
 * user-side different regions might share page-tables, but since they are created only with these
 * 3 operations during which we have acquired the process-lock, it's no problem. Changing PTEs
 * in not-locked-processes is ok anyway.
 * On the kernel-side there is the free area, but this is only used at boot time. The kernel-stacks
 * are created holding the process-lock, so that this works, too. Every other area belongs to
 * exactly one module, which of course takes care of locking itself.
 * In total, there is no need to lock any operation (except temp-page). The reason might be a bit
 * too subtle. TODO should we keep it this way?
 */

ssize_t PageDirBase::mapToCur(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	return Proc::getCurPageDir()->map(virt,frames,count,flags);
}

size_t PageDirBase::unmapFromCur(uintptr_t virt,size_t count,bool freeFrames) {
	return Proc::getCurPageDir()->unmap(virt,count,freeFrames);
}

void PageDirBase::makeFirst() {
	PageDir *pdir = static_cast<PageDir*>(this);
	pdir->pagedir = (uintptr_t)&proc0TLPD & ~KERNEL_AREA;
	pdir->freeKStack = KSTACK_AREA;
	pdir->lock = SpinLock();
}

uintptr_t PageDirBase::makeAccessible(uintptr_t phys,size_t pages) {
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t addr = PageDir::freeAreaAddr;
	if(addr + pages * PAGE_SIZE > KFREE_AREA + KFREE_AREA_SIZE)
		Util::panic("Bootstrap area too small");
	if(phys) {
		for(size_t i = 0; i < pages; ++i) {
			frameno_t frame = phys / PAGE_SIZE + i;
			cur->map(PageDir::freeAreaAddr,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
			PageDir::freeAreaAddr += PAGE_SIZE;
		}
	}
	else {
		cur->map(addr,NULL,pages,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
		PageDir::freeAreaAddr += pages * PAGE_SIZE;
	}
	return addr;
}

uintptr_t PageDir::createKernelStack() {
	uintptr_t addr = freeKStack;
	// take care of overflow
	uintptr_t end = KSTACK_AREA + KSTACK_AREA_SIZE - 1;
	while(addr < end) {
		if(!isPresent(addr)) {
			if(map(addr,NULL,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
				addr = 0;
			break;
		}
		addr += PAGE_SIZE;
	}
	if(addr == end)
		addr = 0;
	else
		freeKStack = addr + PAGE_SIZE;
	return addr;
}

void PageDir::removeKernelStack(uintptr_t addr) {
	if(addr < freeKStack)
		freeKStack = addr;
	unmap(addr,1,true);
}

bool PageDirBase::isPresent(uintptr_t virt) const {
	uintptr_t base;
	const PageDir *pdir = static_cast<const PageDir*>(this);
	PageDir::pte_t *pte = pdir->getPTE(virt,&base);
	return pte ? *pte & PTE_PRESENT : false;
}

frameno_t PageDirBase::getFrameNo(uintptr_t virt) const {
	uintptr_t base = virt;
	const PageDir *pdir = static_cast<const PageDir*>(this);
	PageDir::pte_t *pte = pdir->getPTE(virt,&base);
	assert(pte != NULL);
	return PTE_FRAMENO(*pte) + (virt - base) / PAGE_SIZE;
}

uintptr_t PageDir::mapToTemp(frameno_t frame) {
	PageDir *cur = Proc::getCurPageDir();
	cur->lock.down();
	cur->map(TEMP_MAP_PAGE,&frame,1,
		PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	return TEMP_MAP_PAGE;
}

void PageDir::unmapFromTemp() {
	PageDir *cur = Proc::getCurPageDir();
	cur->lock.up();
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame = Thread::getRunning()->getFrame();
	uintptr_t addr = getAccess(frame);
	memcpy((void*)addr,buffer,loadCount);
	removeAccess(frame);
	return frame;
}

void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	uintptr_t addr = getAccess(frame);
	memcpy((void*)addr,src,PAGE_SIZE);
	removeAccess(frame);
}

void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	uintptr_t addr = getAccess(frame);
	memcpy(dst,(void*)addr,PAGE_SIZE);
	removeAccess(frame);
}

ssize_t PageDirBase::clonePages(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,
                                bool share) {
	PageDir *src = static_cast<PageDir*>(this);
	PageDir *cur = Proc::getCurPageDir();
	size_t pts = 0;
	uintptr_t base,orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	assert(this != dst && (this == cur || dst == cur));
	while(count > 0) {
		PageDir::pte_t *spt = src->getPTE(virtSrc,&base);
		PageDir::pte_t pte = *spt;

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_PRESENT)))
			pte &= ~PTE_WRITABLE;

		int res = dst->mapPage(virtDst,PTE_FRAMENO(pte),pte & ~PTE_FRAMENO_MASK,&pts);
		if(res < 0)
			goto error;
		/* we never need a flush here because it was not present before */

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_PRESENT)) {
			uint flags = pte & ~(PTE_FRAMENO_MASK | PTE_WRITABLE);
			assert(src->mapPage(virtSrc,PTE_FRAMENO(pte),flags,NULL) >= 0);
			if(this == cur)
				FLUSHADDR(virtSrc);
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	SMP::flushTLB(src);
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	dst->unmap(orgVirtDst,orgCount - count,false);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		PageDir::pte_t *pte = src->getPTE(orgVirtSrc,&base);
		if(!share && (*pte & PTE_PRESENT))
			src->mapPage(orgVirtSrc,PTE_FRAMENO(*pte),PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS,NULL);
		orgVirtSrc += PAGE_SIZE;
		orgCount--;
	}
	return -ENOMEM;
}

int PageDir::crtPageTable(PageDir::pte_t *pte,uint flags) {
	frameno_t frame = PhysMem::allocate(PhysMem::KERN);
	if(frame == 0)
		return -ENOMEM;

	assert((*pte & PTE_PRESENT) == 0);
	*pte = frame << PAGE_BITS | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	if(!(flags & PG_SUPERVISOR))
		*pte |= PTE_NOTSUPER;

	memclear((void*)(DIR_MAP_AREA + (frame << PAGE_BITS)),PAGE_SIZE);
	return 0;
}

int PageDir::mapPage(uintptr_t virt,frameno_t frame,uint flags,size_t *pts) {
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + pagedir);
	uint bits = PT_BITS - PT_BPL;
	for(int i = 0; i < PT_LEVELS - 1; ++i) {
		uintptr_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
		if(pt[idx] == 0) {
			if(crtPageTable(pt + idx,flags) < 0)
				return -ENOMEM;
			if(pts)
				(*pts)++;
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

PageDir::pte_t *PageDir::getPTE(uintptr_t virt,uintptr_t *base) const {
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + pagedir);
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

frameno_t PageDir::unmapPage(uintptr_t virt) {
	uintptr_t base = virt;
	pte_t *pte = getPTE(virt,&base);
	assert(pte != NULL && base == virt);
	frameno_t frame = (*pte & PTE_PRESENT) ? PTE_FRAMENO(*pte) : 0;
	*pte = 0;
	return frame;
}

bool PageDir::gc(uintptr_t virt,pte_t pte,int level,uint bits,size_t *pts) {
	if(~pte & PTE_EXISTS)
		return true;
	if(level == 0)
		return false;

	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pte & PTE_FRAMENO_MASK));
	size_t idx = (virt >> bits) & (PT_ENTRY_COUNT - 1);
	if(pt[idx] & PTE_PRESENT) {
		if(!gc(virt,pt[idx],level - 1,bits - PT_BPL,pts))
			return false;

		/* free that page-table */
		PhysMem::free(PTE_FRAMENO(pt[idx]),PhysMem::KERN);
		pt[idx] = 0;
		(*pts)++;
	}

	/* for not-top-level page-tables, check if all other entries are empty now as well */
	if(level < PT_LEVELS) {
		virt &= ~((1 << (bits + PT_BPL)) - 1);
		for(size_t i = 0; i < PT_ENTRY_COUNT; ++i) {
			if((pt[i] & PTE_EXISTS) && !gc(virt,pt[i],level - 1,bits - PT_BPL,pts))
				return false;
			virt += 1 << bits;
		}
	}
	return true;
}

void PageDirBase::freeFrame(uintptr_t virt,frameno_t frame) {
	if(virt >= KERNEL_AREA) {
		if(virt < KSTACK_AREA)
			PhysMem::free(frame,PhysMem::CRIT);
		else
			PhysMem::free(frame,PhysMem::KERN);
	}
	else
		PhysMem::free(frame,PhysMem::USR);
}

ssize_t PageDirBase::map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	PageDir *pdir = static_cast<PageDir*>(this);
	PageDir *cur = Proc::getCurPageDir();
	size_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	PageDir::pte_t pteFlags = PTE_EXISTS;

	virt &= ~(PAGE_SIZE - 1);
	if(flags & PG_PRESENT)
		pteFlags |= PTE_PRESENT;
	if((flags & PG_PRESENT) && (flags & PG_WRITABLE))
		pteFlags |= PTE_WRITABLE;
	if(!(flags & PG_SUPERVISOR))
		pteFlags |= PTE_NOTSUPER;
	if(flags & PG_GLOBAL)
		pteFlags |= PTE_GLOBAL;

	bool needShootdown = false;
	while(count > 0) {
		frameno_t frame = 0;
		if((flags & (PG_KEEPFRM | PG_PRESENT)) == PG_PRESENT) {
			if(frames == NULL) {
				if(virt >= KERNEL_AREA) {
					frame = PhysMem::allocate(virt < KSTACK_AREA ? PhysMem::CRIT : PhysMem::KERN);
					if(frame == 0)
						goto error;
				}
				else
					frame = Thread::getRunning()->getFrame();
			}
			else
				frame = *frames++;
		}

		int res = pdir->mapPage(virt,frame,pteFlags,&pts);
		if(res < 0)
			goto error;
		needShootdown |= res == 1;

		/* invalidate TLB-entry */
		if(this == cur && res == 1)
			FLUSHADDR(virt);

		/* to next page */
		virt += PAGE_SIZE;
		count--;
	}

	if(needShootdown)
		SMP::flushTLB(pdir);
	return pts;

error:
	bool freeFrames = frames == NULL && !(flags & PG_KEEPFRM) && (flags & PG_PRESENT);
	unmap(orgVirt,orgCount - count,freeFrames);
	return -ENOMEM;
}

size_t PageDirBase::unmap(uintptr_t virt,size_t count,bool freeFrames) {
	PageDir *pdir = static_cast<PageDir*>(this);
	PageDir *cur = Proc::getCurPageDir();
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	bool needShootdown = false;
	size_t pts = 0;
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				pdir->gc(virt - PAGE_SIZE,pdir->pagedir | PTE_EXISTS,2,PT_BITS - PT_BPL,&pts);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		frameno_t frame = pdir->unmapPage(virt);
		if(frame) {
			if(freeFrames)
				freeFrame(virt,frame);
			/* invalidate TLB-entry */
			if(this == cur)
				FLUSHADDR(virt);
			needShootdown = true;
		}

		/* to next page */
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
		pdir->gc(virt - PAGE_SIZE,pdir->pagedir | PTE_EXISTS,2,PT_BITS - PT_BPL,&pts);
	if(needShootdown)
		SMP::flushTLB(pdir);
	return pts;
}

size_t PageDir::countEntries(pte_t pte,int level) const {
	size_t count = 0;
	pte_t *pt = reinterpret_cast<pte_t*>(DIR_MAP_AREA + (pte & PTE_FRAMENO_MASK));
	size_t end = level == PT_LEVELS ? ADDR_TO_PDINDEX(KERNEL_AREA) : PT_ENTRY_COUNT;
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

void PageDirBase::print(OStream &os,uint parts) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	if(parts & PD_PART_USER)
		PageDir::printPTE(os,0x00000000,KERNEL_AREA,pdir->pagedir,PT_LEVELS);
	if(parts & PD_PART_KERNEL)
		PageDir::printPTE(os,KERNEL_AREA,0xFFFFFFFF,pdir->pagedir,PT_LEVELS);
}

void PageDir::printPTE(OStream &os,uintptr_t from,uintptr_t to,pte_t page,int level) {
	uint64_t size = (uint64_t)PAGE_SIZE << (PT_BPL * level);
	size_t entrySize = (size_t)PAGE_SIZE << (PT_BPL * (level - 1));
	uintptr_t base = from & ~(size - 1);
	size_t idx = (from >> (PAGE_BITS + PT_BPL * level)) & (PT_ENTRY_COUNT - 1);
	os.writef("%*s%#03x: r=%#08x [%c%c%c%c%c] (VM: %p - %p)\n",(PT_LEVELS - level) * 2,"",
			idx,page,
			(page & PTE_PRESENT) ? 'p' : '-',(page & PTE_NOTSUPER) ? 'u' : 'k',
			(page & PTE_WRITABLE) ? 'w' : 'r',(page & PTE_GLOBAL) ? 'g' : '-',
			(page & PTE_LARGE) ? 'l' : '-',
			base,base + size - 1);
	if(level > 0 && (~page & PTE_LARGE)) {
		ulong *pt = reinterpret_cast<ulong*>(DIR_MAP_AREA + (page & PTE_FRAMENO_MASK));
		size_t end,start = from >> (PAGE_BITS + PT_BPL * (level - 1)) & (PT_ENTRY_COUNT - 1);
		uintptr_t last = to + entrySize - 1;
		if(MAX(to,last) >= base + entrySize * PT_ENTRY_COUNT - 1)
			end = PT_ENTRY_COUNT;
		else
			end = last >> (PAGE_BITS + PT_BPL * (level - 1)) & (PT_ENTRY_COUNT - 1);
		for(; start < end; ++start) {
			if(pt[start] & PTE_EXISTS)
				printPTE(os,from,MIN(from + entrySize - 1,to),pt[start],level - 1);
			from += entrySize;
		}
	}
}
