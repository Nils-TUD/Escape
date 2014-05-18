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
#include <sys/mem/physmem.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/physmemareas.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* to shift a flag down to the first bit */
#define PG_PRESENT_SHIFT			0
#define PG_WRITABLE_SHIFT			1

#define TLB_SIZE					32
#define TLB_FIXED					4

#define PAGE_DIR_DIRMAP				(PageDir::curPDir)
#define PAGE_DIR_DIRMAP_OF(pdir) 	(pdir)

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr)		(MAPPED_PTS_START + \
		(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + \
		(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))

uintptr_t PageDir::curPDir = 0;
uintptr_t PageDir::otherPDir = 0;

void PageDirBase::init() {
	/* set page-dir of first process */
	frameno_t frame = PhysMem::allocate(PhysMem::KERN);
	if(frame == INVALID_FRAME)
		Util::panic("Not enough memory for initial page-dir");
	PageDir::curPDir = (frame * PAGE_SIZE) | DIR_MAP_AREA;
	PageDir::PDEntry *pd = (PageDir::PDEntry*)PageDir::curPDir;
	/* clear */
	memclear(pd,PAGE_SIZE);

	/* put the page-directory in itself */
	PageDir::PDEntry *pde = pd + ADDR_TO_PDINDEX(MAPPED_PTS_START);
	pde->ptFrameNo = (PageDir::curPDir & ~DIR_MAP_AREA) / PAGE_SIZE;
	pde->present = true;
	pde->writable = true;
	pde->exists = true;

	/* insert shared page-tables */
	uintptr_t addr = KHEAP_START;
	uintptr_t end = KERNEL_STACK;
	pde = pd + ADDR_TO_PDINDEX(KHEAP_START);
	while(addr < end) {
		/* get frame and insert into page-dir */
		frame = PhysMem::allocate(PhysMem::KERN);
		if(frame == INVALID_FRAME)
			Util::panic("Not enough memory for kernel page-tables");
		pde->ptFrameNo = frame;
		pde->present = true;
		pde->writable = true;
		pde->exists = true;
		/* clear */
		memclear((void*)((frame * PAGE_SIZE) | DIR_MAP_AREA),PAGE_SIZE);
		/* to next */
		pde++;
		addr += PAGE_SIZE * PT_ENTRY_COUNT;
	}
}

int PageDirBase::mapToCur(uintptr_t virt,size_t count,Allocator &alloc,uint flags) {
	return Proc::getCurPageDir()->map(virt,count,alloc,flags);
}

void PageDirBase::unmapFromCur(uintptr_t virt,size_t count,Allocator &alloc) {
	Proc::getCurPageDir()->unmap(virt,count,alloc);
}

int PageDirBase::cloneKernelspace(PageDir *pdir,A_UNUSED tid_t tid) {
	/* allocate frames */
	frameno_t pdirFrame = PhysMem::allocate(PhysMem::KERN);
	if(pdirFrame == INVALID_FRAME)
		return -ENOMEM;
	frameno_t stackPtFrame = PhysMem::allocate(PhysMem::KERN);
	if(stackPtFrame == INVALID_FRAME) {
		PhysMem::free(pdirFrame,PhysMem::KERN);
		return -ENOMEM;
	}

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	PageDir::PDEntry *pd = (PageDir::PDEntry*)PAGE_DIR_DIRMAP;
	PageDir::PDEntry *npd = (PageDir::PDEntry*)((pdirFrame * PAGE_SIZE) | DIR_MAP_AREA);

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(PageDir::PDEntry));
	/* copy kernel-space page-tables (beginning to temp-map-area, inclusive) */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE - KERNEL_AREA) /
				(PAGE_SIZE * PT_ENTRY_COUNT) * sizeof(PageDir::PDEntry));
	/* clear the rest */
	memclear(npd + ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE))
			* sizeof(PageDir::PDEntry));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	PageDir::PDEntry *tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK);
	tpd->ptFrameNo = stackPtFrame;
	tpd->present = true;
	tpd->writable = true;
	tpd->exists = true;
	/* clear the page-table */
	PageDir::otherPDir = 0;
	PageDir::PTEntry *pt = (PageDir::PTEntry*)((tpd->ptFrameNo * PAGE_SIZE) | DIR_MAP_AREA);
	memclear(pt,PAGE_SIZE);

	pdir->phys = DIR_MAP_AREA | (pdirFrame << PAGE_BITS);
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	assert(pdir->phys != PageDir::curPDir);
	/* free page-table for kernel-stack */
	PageDir::PDEntry *pde = (PageDir::PDEntry*)PAGE_DIR_DIRMAP_OF(pdir->phys)
			+ ADDR_TO_PDINDEX(KERNEL_STACK);
	pde->present = false;
	pde->exists = false;
	PhysMem::free(pde->ptFrameNo,PhysMem::KERN);
	/* free page-dir */
	PhysMem::free((pdir->phys & ~DIR_MAP_AREA) >> PAGE_BITS,PhysMem::KERN);
	/* ensure that we don't use it again */
	PageDir::otherPDir = 0;
}

bool PageDirBase::isPresent(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t ptables = pdir->getPTables();
	PageDir::PDEntry *pde = (PageDir::PDEntry*)PAGE_DIR_DIRMAP_OF(pdir->phys) + ADDR_TO_PDINDEX(virt);
	if(!pde->present || !pde->exists)
		return false;
	PageDir::PTEntry *pt = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	return pt->present && pt->exists;
}

frameno_t PageDirBase::getFrameNo(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t ptables = pdir->getPTables();
	PageDir::PTEntry *pt = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	assert(pt->present && pt->exists);
	return pt->frameNumber;
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame = Thread::getRunning()->getFrame();
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAP_AREA),buffer,loadCount);
	return frame;
}

void PageDirBase::copyToUser(void *dst,const void *src,size_t count) {
	PageDir::PTEntry *pt = (PageDir::PTEntry*)ADDR_TO_MAPPED(dst);
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	while(count > 0) {
		size_t amount = MIN(PAGE_SIZE - offset,count);
		uintptr_t addr = ((pt->frameNumber << PAGE_BITS) | DIR_MAP_AREA) + offset;
		memcpy((void*)addr,src,amount);
		src = (const void*)((uintptr_t)src + amount);
		count -= amount;
		offset = 0;
		pt++;
	}
}

void PageDirBase::zeroToUser(void *dst,size_t count) {
	PageDir::PTEntry *pt = (PageDir::PTEntry*)ADDR_TO_MAPPED(dst);
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	while(count > 0) {
		size_t amount = MIN(PAGE_SIZE - offset,count);
		uintptr_t addr = ((pt->frameNumber << PAGE_BITS) | DIR_MAP_AREA) + offset;
		memclear((void*)addr,amount);
		count -= amount;
		offset = 0;
		pt++;
	}
}

int PageDirBase::clonePages(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,
                                bool share) {
	NoAllocator alloc;
	int pts = 0;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	PageDir *src = static_cast<PageDir*>(this);
	uintptr_t srctables = src->getPTables();
	assert(src != dst && (src->phys == PageDir::curPDir || dst->phys == PageDir::curPDir));
	while(count > 0) {
		PageDir::PTEntry *pte = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(srctables,virtSrc);
		frameno_t frame = 0;
		uint flags = 0;
		if(pte->present)
			flags |= PG_PRESENT;
		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if(pte->writable && (share || !pte->present))
			flags |= PG_WRITABLE;
		if(share || pte->present)
			frame = pte->frameNumber;

		{
			RangeAllocator rngalloc(frame);
			ssize_t mres = dst->map(virtDst,1,rngalloc,flags);
			if(mres < 0)
				goto error;
			pts += rngalloc.pageTables();
		}

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && pte->present)
			src->map(virtSrc,1,alloc,flags);
		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	dst->unmap(orgVirtDst,orgCount - count,alloc);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		PageDir::PTEntry *pte = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(srctables,orgVirtSrc);
		if(!share && pte->present)
			src->map(orgVirtSrc,1,alloc,PG_PRESENT | PG_WRITABLE);
		orgVirtSrc += PAGE_SIZE;
		orgCount--;
	}
	return -ENOMEM;
}

int PageDirBase::map(uintptr_t virt,size_t count,Allocator &alloc,uint flags) {
	PageDir *pdir = static_cast<PageDir*>(this);
	ssize_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	uintptr_t ptables = pdir->getPTables();
	uintptr_t pdirAddr = PAGE_DIR_DIRMAP_OF(pdir->phys);

	virt &= ~(PAGE_SIZE - 1);
	while(count > 0) {
		PageDir::PTEntry *pte;
		PageDir::PDEntry *pde = (PageDir::PDEntry*)pdirAddr + ADDR_TO_PDINDEX(virt);
		/* page table not present? */
		if(!pde->present) {
			/* get new frame for page-table */
			pde->ptFrameNo = alloc.allocPT();
			if(pde->ptFrameNo == INVALID_FRAME)
				goto error;
			pde->present = true;
			pde->exists = true;
			/* writable because we want to be able to change PTE's in the PTE-area */
			pde->writable = true;
			pts++;

			PageDir::flushPageTable(virt,ptables);
			/* clear frame */
			memclear((void*)((pde->ptFrameNo * PAGE_SIZE) | DIR_MAP_AREA),PAGE_SIZE);
		}

		/* setup page */
		pte = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		/* ignore already present entries */
		pte->present = (flags & PG_PRESENT) >> PG_PRESENT_SHIFT;
		pte->exists = true;
		pte->writable = (flags & PG_WRITABLE) >> PG_WRITABLE_SHIFT;

		/* set frame-number */
		if(flags & PG_PRESENT) {
			pte->frameNumber = alloc.allocPage();
			if(pte->frameNumber == INVALID_FRAME)
				goto error;
		}

		/* invalidate TLB-entry */
		if(pdir->phys == PageDir::curPDir)
			PageDir::tlbRemove(virt);

		/* to next page */
		virt += PAGE_SIZE;
		count--;
	}

	return pts;

error:
	pdir->unmap(orgVirt,orgCount - count,alloc);
	return -ENOMEM;
}

void PageDirBase::freeFrame(A_UNUSED uintptr_t virt,frameno_t frame) {
	PhysMem::free(frame,PhysMem::USR);
}

void PageDirBase::unmap(uintptr_t virt,size_t count,Allocator &alloc) {
	PageDir *pdir = static_cast<PageDir*>(this);
	uintptr_t ptables = pdir->getPTables();
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	PageDir::PTEntry *pte = (PageDir::PTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				pdir->remEmptyPt(ptables,lastPti,alloc);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		if(pte->present) {
			pte->present = false;
			alloc.freePage(pte->frameNumber);
		}
		pte->exists = false;

		/* invalidate TLB-entry */
		if(pdir->phys == PageDir::curPDir)
			PageDir::tlbRemove(virt);

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
		pdir->remEmptyPt(ptables,pti,alloc);
}

size_t PageDir::remEmptyPt(uintptr_t ptables,size_t pti,Allocator &alloc) {
	uintptr_t virt = pti * PAGE_SIZE * PT_ENTRY_COUNT;
	PTEntry *pte = (PTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pte[i].exists)
			return 0;
	}
	/* empty => can be removed */
	PDEntry *pde = (PDEntry*)PAGE_DIR_DIRMAP_OF(phys) + pti;
	pde->present = false;
	pde->exists = false;
	alloc.freePT(pde->ptFrameNo);
	if(ptables == MAPPED_PTS_START)
		flushPageTable(virt,ptables);
	else
		tlbRemove((uintptr_t)pte);
	return 1;
}

void PageDir::flushPageTable(uintptr_t virt,uintptr_t ptables) {
	uintptr_t mapAddr = ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	/* to beginning of page-table */
	virt &= ~(PT_ENTRY_COUNT * PAGE_SIZE - 1);
	for(int i = TLB_FIXED; i < TLB_SIZE; i++) {
		uint entryHi,entryLo;
		tlbGet(i,&entryHi,&entryLo);
		/* affected by the page-table? */
		if((entryHi >= virt && entryHi < virt + PAGE_SIZE * PT_ENTRY_COUNT) || entryHi == mapAddr)
			tlbSet(i,DIR_MAP_AREA,0);
	}
}

uintptr_t PageDir::getPTables() const {
	if(phys == curPDir)
		return MAPPED_PTS_START;
	if(phys == otherPDir)
		return TMPMAP_PTS_START;
	/* map page-tables to other area*/
	PDEntry *pde = ((PDEntry*)PAGE_DIR_DIRMAP) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	pde->present = true;
	pde->exists = true;
	pde->ptFrameNo = (phys & ~DIR_MAP_AREA) >> PAGE_BITS;
	pde->writable = true;
	/* we want to access the whole page-table */
	flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	otherPDir = const_cast<PageDir*>(this)->phys;
	return TMPMAP_PTS_START;
}

size_t PageDirBase::getPageCount() const {
	size_t count = 0;
	PageDir::PDEntry *pdir = (PageDir::PDEntry*)PAGE_DIR_DIRMAP;
	for(size_t i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdir[i].present) {
			PageDir::PTEntry *pagetable = (PageDir::PTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(size_t x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pagetable[x].present)
					count++;
			}
		}
	}
	return count;
}

void PageDir::printTLB(OStream &os) const {
	os.writef("TLB:\n");
	for(int i = 0; i < TLB_SIZE; i++) {
		uint entryHi,entryLo;
		tlbGet(i,&entryHi,&entryLo);
		os.writef("\t%d: %08x %08x %c%c\n",i,entryHi,entryLo,
			(entryLo & 0x2) ? 'w' : '-',(entryLo & 0x1) ? 'v' : '-');
	}
}

void PageDirBase::print(OStream &os,uint parts) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t ptables = pdir->getPTables();
	PageDir::PDEntry *pdirAddr = (PageDir::PDEntry*)PAGE_DIR_DIRMAP_OF(pdir->phys);
	os.writef("page-dir @ 0x%08x:\n",pdirAddr);
	for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
		if(!pdirAddr[i].present)
			continue;
		if(parts == PD_PART_ALL ||
			(i < ADDR_TO_PDINDEX(KERNEL_AREA) && (parts & PD_PART_USER)) ||
			(i >= ADDR_TO_PDINDEX(KERNEL_AREA) &&
					i < ADDR_TO_PDINDEX(KHEAP_START) &&
					(parts & PD_PART_KERNEL)) ||
			(i >= ADDR_TO_PDINDEX(TEMP_MAP_AREA) &&
					i <= ADDR_TO_PDINDEX(KERNEL_STACK) &&
					(parts & TEMP_MAP_AREA)) ||
			(i >= ADDR_TO_PDINDEX(KHEAP_START) &&
					i < ADDR_TO_PDINDEX(KHEAP_START + KHEAP_SIZE) &&
					(parts & PD_PART_KHEAP)) ||
			(i >= ADDR_TO_PDINDEX(MAPPED_PTS_START) && (parts & PD_PART_PTBLS))) {
			PageDir::printPageTable(os,ptables,i,pdirAddr + i);
		}
	}
	os.writef("\n");
}

void PageDir::printPageTable(OStream &os,uintptr_t ptables,size_t no,PDEntry *pde) {
	uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	PTEntry *pte = (PTEntry*)(ptables + no * PAGE_SIZE);
	os.writef("\tpt 0x%x [frame 0x%x, %c] @ 0x%08Px: (VM: 0x%08Px - 0x%08Px)\n",no,
			pde->ptFrameNo,pde->writable ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].exists) {
				os.writef("\t\t0x%zx: ",i);
				printPage(os,pte + i);
				os.writef(" (VM: 0x%08Px - 0x%08Px)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

void PageDir::printPage(OStream &os,PTEntry *page) {
	if(page->exists) {
		os.writef("r=0x%08x fr=0x%x [%c%c]",*(uint32_t*)page,
				page->frameNumber,page->present ? 'p' : '-',page->writable ? 'w' : 'r');
	}
	else {
		os.writef("-");
	}
}
