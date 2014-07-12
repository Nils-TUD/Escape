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
#include <mem/pagedir.h>
#include <mem/physmem.h>
#include <mem/virtmem.h>
#include <mem/cache.h>
#include <mem/physmemareas.h>
#include <task/proc.h>
#include <task/thread.h>
#include <cpu.h>
#include <util.h>
#include <video.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

PageDir PageDir::firstCon;

void PageDirBase::init() {
	ssize_t res;
	/* set root-location of first process */
	if((res = PhysMem::allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1)) < 0)
		Util::panic("Not enough contiguous memory for the root-location of the first process");
	uintptr_t rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAP_AREA;
	/* clear */
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* create context for the first process */
	PageDir::firstCon.addrSpace = AddressSpace::alloc();
	/* set value for rV: b1 = 2, b2 = 4, b3 = 6, b4 = 0, page-size = 2^13 */
	PageDir::firstCon.rv = 0x24600DUL << 40 | (rootLoc & ~DIR_MAP_AREA) |
			(PageDir::firstCon.addrSpace->getNo() << 3);
	/* enable paging */
	PageDir::setrV(PageDir::firstCon.rv);
}

int PageDirBase::cloneKernelspace(PageDir *pdir,A_UNUSED tid_t tid) {
	PageDir *cur = Proc::getCurPageDir();
	/* allocate root-location */
	ssize_t res = PhysMem::allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1);
	if(res < 0)
		return res;
	/* clear */
	uintptr_t rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAP_AREA;
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* init context */
	pdir->addrSpace = AddressSpace::alloc();
	pdir->ptables = 0;
	pdir->rv = cur->rv & 0xFFFFFF0000000000;
	pdir->rv |= (rootLoc & ~DIR_MAP_AREA) | (pdir->addrSpace->getNo() << 3);
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	assert(pdir != Proc::getCurPageDir());
	/* free page-dir */
	PhysMem::freeContiguous((pdir->rv >> PAGE_BITS) & 0x7FFFFFF,SEGMENT_COUNT * PTS_PER_SEGMENT);
	/* free address-space */
	AddressSpace::free(pdir->addrSpace);
	/* we have to ensure that no tc-entries of the current process are present. otherwise we could
	 * get the problem that the old process had a page that the new process with this
	 * address-space-number has not. if the entry of the old one is still in the tc, the new
	 * process could access it and write to that frame (which might be used for a different
	 * purpose now) */
	PageDir::clearTC();
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,ulong regFlags) {
	/* copy into frame */
	frameno_t frame = Thread::getRunning()->getFrame();
	uintptr_t addr = frame * PAGE_SIZE | DIR_MAP_AREA;
	memcpy((void*)addr,buffer,loadCount);
	/* if its an executable region, we have to syncid the memory afterwards */
	if(regFlags & RF_EXECUTABLE)
		CPU::syncid(addr,addr + loadCount);
	return frame;
}

void PageDirBase::zeroToUser(void *dst,size_t count) {
	uint64_t pte,*dpt = NULL;
	ulong dstPageNo = PAGE_NO(dst);
	PageDir *cur = Proc::getCurPageDir();
	PageTables::NoAllocator alloc;
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	uintptr_t addr = (uintptr_t)dst;
	while(count > 0) {
		if(!dpt || (dstPageNo % PT_ENTRY_COUNT) == 0) {
			dpt = cur->getPT(addr,false,alloc);
			assert(dpt != NULL);
		}
		pte = dpt[dstPageNo % PT_ENTRY_COUNT];
		addr = ((pte & PTE_FRAMENO_MASK) | DIR_MAP_AREA) + offset;

		size_t amount = MIN(PAGE_SIZE - offset,count);
		memclear((void*)addr,amount);
		count -= amount;
		offset = 0;
		dstPageNo++;
	}
}

int PageDirBase::clone(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,bool share) {
	PageDir *cur = Proc::getCurPageDir();
	PageDir *src = static_cast<PageDir*>(this);
	PageTables::NoAllocator alloc;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	ulong srcPageNo = PAGE_NO(virtSrc);
	ulong dstPageNo = PAGE_NO(virtDst);
	uint64_t pte,*spt = NULL,*dpt = NULL;
	uint64_t dstAddrSpace = dst->addrSpace->getNo() << 3;
	uint64_t dstKey = virtDst | dstAddrSpace;
	uint64_t srcKey = virtSrc | (src->addrSpace->getNo() << 3);
	assert(this != dst && (this == cur || dst == cur));
	while(count > 0) {
		/* get src-page-table */
		if(!spt || (srcPageNo % PT_ENTRY_COUNT) == 0) {
			spt = src->getPT(virtSrc,false,alloc);
			assert(spt != NULL);
		}
		/* get dest-page-table */
		if(!dpt || (dstPageNo % PT_ENTRY_COUNT) == 0) {
			dpt = dst->getPT(virtDst,true,alloc);
			if(!dpt)
				goto error;
		}
		pte = spt[srcPageNo % PT_ENTRY_COUNT];

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_READABLE)))
			pte &= ~PTE_WRITABLE;
		pte &= ~PTE_NMASK;
		pte |= dstAddrSpace;
		dpt[dstPageNo % PT_ENTRY_COUNT] = pte;
		PageDir::updateTC(dstKey);

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_READABLE)) {
			spt[srcPageNo % PT_ENTRY_COUNT] &= ~PTE_WRITABLE;
			PageDir::updateTC(srcKey | (pte & (PTE_READABLE | PTE_EXECUTABLE)));
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		srcKey += PAGE_SIZE;
		dstKey += PAGE_SIZE;
		srcPageNo++;
		dstPageNo++;
		count--;
	}
	dst->ptables += alloc.pageTables();
	return alloc.pageTables();

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	dst->unmap(orgVirtDst,orgCount - count,alloc);
	/* make the cow-pages writable again */
	spt = NULL;
	srcPageNo = PAGE_NO(orgVirtSrc);
	while(orgCount > count) {
		if(!spt || (srcPageNo % PT_ENTRY_COUNT) == 0) {
			spt = src->getPT(orgVirtSrc,false,alloc);
			assert(spt != NULL);
		}
		pte = spt[srcPageNo % PT_ENTRY_COUNT];
		if(!share && (pte & PTE_READABLE)) {
			spt[srcPageNo % PT_ENTRY_COUNT] |= PTE_WRITABLE;
			PageDir::updateTC(srcKey | (pte & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE)));
		}
		orgVirtSrc += PAGE_SIZE;
		srcPageNo++;
		orgCount--;
	}
	return -ENOMEM;
}

int PageDirBase::mapToCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	return Proc::getCurPageDir()->map(virt,count,alloc,flags);
}

int PageDirBase::map(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	PageDir *pdir = static_cast<PageDir*>(this);
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	ulong pageNo = PAGE_NO(virt);
	uint64_t key,pteFlags = 0;
	uint64_t pteAttr = 0;
	uint64_t pte,oldFrame,*pt = NULL;

	virt &= ~(PAGE_SIZE - 1);
	if(flags & PG_PRESENT)
		pteFlags |= PTE_READABLE;
	if((flags & PG_PRESENT) && (flags & PG_WRITABLE))
		pteFlags |= PTE_WRITABLE;
	if((flags & PG_PRESENT) && (flags & PG_EXECUTABLE))
		pteFlags |= PTE_EXECUTABLE;
	pteAttr = pteFlags | (pdir->addrSpace->getNo() << 3);
	key = virt | pteAttr;
	pteAttr |= PTE_EXISTS;

	while(count-- > 0) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			pt = pdir->getPT(virt,true,alloc);
			if(!pt)
				goto error;
		}

		oldFrame = pt[pageNo % PT_ENTRY_COUNT] & PTE_FRAMENO_MASK;
		pte = pteAttr;
		if(flags & PG_PRESENT) {
			frameno_t frame = alloc.allocPage();
			if(frame == INVALID_FRAME)
				goto error;
			if(frame == 0)
				pte |= oldFrame;
			else
				pte |= frame << PAGE_BITS;
		}
		pt[pageNo % PT_ENTRY_COUNT] = pte;

		/* update entries in TCs; protection-flags might have changed */
		/* we have to do that for not running processes as well, since their entry might still
		 * be in the TCs */
		/* note: if the frame has changed, we have to remove the entry instead of replacing the
		 * protection-flags */
		if(oldFrame == (pte & PTE_FRAMENO_MASK))
			PageDir::updateTC(key);
		else
			PageDir::updateTC(key & ~(PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE));

		/* to next page */
		virt += PAGE_SIZE;
		key += PAGE_SIZE;
		pageNo++;
	}
	pdir->ptables += alloc.pageTables();
	return 0;

error:
	pdir->unmap(orgVirt,orgCount - count,alloc);
	return -ENOMEM;
}

void PageDirBase::unmapFromCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	Proc::getCurPageDir()->unmap(virt,count,alloc);
}

void PageDirBase::unmap(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	PageDir *pdir = static_cast<PageDir*>(this);
	ulong pageNo = PAGE_NO(virt);
	uint64_t pte,*pt = NULL;
	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			/* not the first one? then check if its empty */
			if(pt)
				pdir->remEmptyPts(virt - PAGE_SIZE,alloc);
			pt = pdir->getPT(virt,false,alloc);
			assert(pt != NULL);
		}

		/* remove page and free if necessary */
		pte = pt[pageNo % PT_ENTRY_COUNT];
		if(pte & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE))
			alloc.freePage(PTE_FRAMENO(pte));
		pt[pageNo % PT_ENTRY_COUNT] = 0;

		/* to next page */
		virt += PAGE_SIZE;
		pageNo++;
	}
	/* check if the last changed pagetable is empty (pt is NULL if no pages have been unmapped) */
	if(pt)
		pdir->remEmptyPts(virt - PAGE_SIZE,alloc);
	pdir->ptables -= alloc.pageTables();
}

uint64_t *PageDir::getPT(uintptr_t virt,bool create,PageTables::Allocator &alloc) const {
	ulong j,i = virt >> 61;
	ulong size1 = SEGSIZE(rv,i + 1);
	ulong size2 = SEGSIZE(rv,i);
	uint64_t c,pageNo = PAGE_NO(virt);
	uint64_t limit = 1UL << (10 * (size1 - size2));
	if(size1 < size2 || pageNo >= limit)
		return NULL;

	j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = PAGE_NO(virt);
	c = ((rv & 0xFFFFFFFFFF) >> 13) + size2 + j;
	for(; j > 0; j--) {
		ulong ax = (pageNo >> (10 * j)) & 0x3FF;
		uint64_t *ptpAddr = (uint64_t*)(DIR_MAP_AREA | ((c << 13) + (ax << 3)));
		uint64_t ptp = *ptpAddr;
		if(ptp == 0) {
			if(!create)
				return NULL;
			/* allocate page-table and clear it */
			frameno_t frame = alloc.allocPT();
			if(frame == INVALID_FRAME)
				return NULL;
			*ptpAddr = DIR_MAP_AREA | (frame * PAGE_SIZE);
			memclear((void*)*ptpAddr,PAGE_SIZE);
			/* put rV.n in that page-table */
			*ptpAddr |= addrSpace->getNo() << 3;
			ptp = *ptpAddr;
		}
		c = (ptp & ~DIR_MAP_AREA) >> 13;
	}
	return (uint64_t*)(DIR_MAP_AREA | (c << 13));
}

uint64_t PageDir::getPTE(uintptr_t virt) const {
	PageTables::NoAllocator alloc;
	uint64_t pte = 0;
	uint64_t *pt = getPT(virt,false,alloc);
	if(pt)
		return pt[PAGE_NO(virt) % PT_ENTRY_COUNT];
	return pte;
}

size_t PageDir::removePts(uint64_t pageNo,uint64_t c,ulong level,ulong depth,PageTables::Allocator &alloc) {
	bool empty = true;
	size_t count = 0;
	/* page-table with PTPs? */
	if(level > 0) {
		ulong ax = (pageNo >> (10 * level)) & 0x3FF;
		uint64_t *ptpAddr = (uint64_t*)(DIR_MAP_AREA | ((c << 13) + (ax << 3)));
		count = removePts(pageNo,(*ptpAddr & ~DIR_MAP_AREA) >> 13,level - 1,depth + 1,alloc);
		/* if count is equal to level, we know that all deeper page-tables have been free'd.
		 * therefore, we can remove the entry in our page-table */
		if(count == level)
			*ptpAddr = 0;
		/* don't free frames in the root-location */
		if(depth > 0) {
			/* now go through our page-table and check whether there still are used entries */
			uint64_t *ptp = (uint64_t*)(DIR_MAP_AREA | (c << 13));
			for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
				if(*ptp != 0) {
					empty = false;
					break;
				}
				ptp++;
			}
			/* free the frame, if the pt is empty */
			if(empty) {
				alloc.freePT(c);
				count++;
			}
		}
	}
	else if(depth > 0) {
		/* go through our page-table and check whether there still are used entries */
		uint64_t *pte = (uint64_t*)(DIR_MAP_AREA | (c << 13));
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(*pte & PTE_EXISTS) {
				empty = false;
				break;
			}
			pte++;
		}
		/* free the frame, if the pt is empty */
		if(empty) {
			/* remove all pages in that page-table from the TCs */
			tcRemPT(pageNo * PAGE_SIZE);
			alloc.freePT(c);
			count++;
		}
	}
	return count;
}

size_t PageDir::remEmptyPts(uintptr_t virt,PageTables::Allocator &alloc) {
	ulong i = virt >> 61;
	uint64_t pageNo = PAGE_NO(virt);
	int j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_BITS;
	uint64_t c = ((rv & 0xFFFFFFFFFF) >> 13) + SEGSIZE(rv,i) + j;
	return removePts(pageNo,c,j,0,alloc);
}

void PageDir::tcRemPT(uintptr_t virt) {
	uint64_t key = ROUND_PAGE_DN(virt) | (addrSpace->getNo() << 3);
	for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
		updateTC(key);
		key += PAGE_SIZE;
	}
}

void PageDirBase::print(OStream &os,A_UNUSED uint parts) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t root = DIR_MAP_AREA | (pdir->rv & 0xFFFFFFE000);
	/* go through all page-tables in the root-location */
	os.writef("root-location @ %p [n=%X]:\n",root,(pdir->rv & 0x1FF8) >> 3);
	for(size_t i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(pdir->rv,i + 1) - SEGSIZE(pdir->rv,i);
		uintptr_t addr = (i << 61);
		os.writef("segment %zu:\n",i);
		for(size_t j = 0; j < segSize; j++) {
			PageDir::printPageTable(os,i,addr,(uint64_t*)root,j,1);
			addr = (i << 61) | (PAGE_SIZE * (1UL << (10 * (j + 1))));
			root += PAGE_SIZE;
		}
	}
	os.writef("\n");
}

void PageDir::printPageTable(OStream &os,ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent) {
	if(level > 0) {
		/* page-table with PTPs */
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0) {
				os.writef("%*sPTP%zd[%zd]=%PX n=%X (VM: %p - %p)\n",indent * 2,"",level,i,
						(pt[i] & ~DIR_MAP_AREA) / PAGE_SIZE,(pt[i] & PTE_NMASK) >> 3,
						addr,addr + PAGE_SIZE * (1UL << (10 * level)) - 1);
				printPageTable(os,seg,addr,(uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1,indent + 1);
			}
			addr += PAGE_SIZE * (1UL << (10 * level));
		}
	}
	else {
		/* page-table with PTEs */
		uint64_t *pte = (uint64_t*)pt;
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & PTE_EXISTS) {
				os.writef("%*s%zx: ",indent * 2,"",i);
				printPTE(os,pte[i]);
				os.writef(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

void PageDir::printPTE(OStream &os,uint64_t pte) {
	if(pte & PTE_EXISTS) {
		os.writef("f=%PX n=%X [%c%c%c]",PTE_FRAMENO(pte),(pte & PTE_NMASK) >> 3,
				(pte & PTE_READABLE) ? 'r' : '-',
				(pte & PTE_WRITABLE) ? 'w' : '-',
				(pte & PTE_EXECUTABLE) ? 'x' : '-');
	}
	else {
		os.writef("-");
	}
}

size_t PageDir::getPageCountOf(uint64_t *pt,size_t level) {
	size_t count = 0;
	if(level > 0) {
		/* page-table with PTPs */
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0)
				count += getPageCountOf((uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1);
		}
	}
	else {
		/* page-table with PTEs */
		uint64_t *pte = (uint64_t*)pt;
		for(size_t i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE))
				count++;
		}
	}
	return count;
}

size_t PageDirBase::getPageCount() const {
	size_t count = 0;
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t root = DIR_MAP_AREA | (cur->rv & 0xFFFFFFE000);
	for(size_t i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(cur->rv,i + 1) - SEGSIZE(cur->rv,i);
		for(size_t j = 0; j < segSize; j++) {
			count += PageDir::getPageCountOf((uint64_t*)root,j);
			root += PAGE_SIZE;
		}
	}
	return count;
}
