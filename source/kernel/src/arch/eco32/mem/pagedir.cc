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
#include <mem/physmem.h>
#include <mem/physmemareas.h>
#include <mem/virtmem.h>
#include <task/proc.h>
#include <task/thread.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <string.h>
#include <util.h>
#include <video.h>

#define TLB_SIZE					32
#define TLB_FIXED					4

uintptr_t PageDir::curPDir = 0;

void PageDirBase::init() {
	/* set page-dir of first process */
	frameno_t frame = PhysMem::allocate(PhysMem::KERN);
	if(frame == INVALID_FRAME)
		Util::panic("Not enough memory for initial page-dir");
	PageDir::curPDir = (frame * PAGE_SIZE) | DIR_MAP_AREA;

	/* clear */
	pte_t *pd = (pte_t*)PageDir::curPDir;
	memclear(pd,PAGE_SIZE);

	/* insert pagedir in itself */
	pd[PT_IDX(MAPPED_PTS_START,1)] = (frame << PAGE_BITS) | PTE_EXISTS | PTE_PRESENT | PTE_WRITABLE;

	/* create kernel page-tables */
	PageDir pdir;
	pdir.pts.setRoot(frame * PAGE_SIZE);
	/* use the stack allocator here to create page-tables */
	PageTables::KStackAllocator alloc;
	pdir.map(KHEAP_START,(KERNEL_STACK - KHEAP_START) / PAGE_SIZE,alloc,PG_NOPAGES | PG_SUPERVISOR);
}

int PageDirBase::mapToCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc,uint flags) {
	return Proc::getCurPageDir()->map(virt,count,alloc,flags);
}

void PageDirBase::unmapFromCur(uintptr_t virt,size_t count,PageTables::Allocator &alloc) {
	Proc::getCurPageDir()->unmap(virt,count,alloc);
}

int PageDirBase::cloneKernelspace(PageDir *pdir,A_UNUSED tid_t tid) {
	PageDir *cur = Proc::getCurPageDir();

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
	pte_t *pd = (pte_t*)(DIR_MAP_AREA + cur->pts.getRoot());
	pte_t *npd = (pte_t*)((pdirFrame * PAGE_SIZE) | DIR_MAP_AREA);

	/* clear user-space page-tables */
	memclear(npd,PT_IDX(KERNEL_AREA,1) * sizeof(pte_t));
	/* copy kernel-space page-tables (beginning to temp-map-area, inclusive) */
	memcpy(npd + PT_IDX(KERNEL_AREA,1),
			pd + PT_IDX(KERNEL_AREA,1),
			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE - KERNEL_AREA) /
				(PAGE_SIZE * PT_ENTRY_COUNT) * sizeof(pte_t));
	/* clear the rest */
	memclear(npd + PT_IDX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE,1),
			(PT_ENTRY_COUNT - PT_IDX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE,1))
			* sizeof(pte_t));

	/* insert pagedir in itself */
	npd[PT_IDX(MAPPED_PTS_START,1)] = (pdirFrame << PAGE_BITS) | PTE_PRESENT | PTE_WRITABLE;

	/* get new page-table for the kernel-stack-area and the stack itself */
	npd[PT_IDX(KERNEL_STACK,1)] =
			stackPtFrame << PAGE_BITS | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	/* clear the page-table */
	memclear((void*)(DIR_MAP_AREA + (stackPtFrame << PAGE_BITS)),PAGE_SIZE);

	pdir->pts.setRoot(pdirFrame << PAGE_BITS);
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	assert((DIR_MAP_AREA + pdir->pts.getRoot()) != PageDir::curPDir);
	/* free page-table for kernel-stack */
	pte_t *pte = (pte_t*)(DIR_MAP_AREA + pdir->pts.getRoot());
	PhysMem::free(PTE_FRAMENO(pte[PT_IDX(KERNEL_STACK,1)]),PhysMem::KERN);
	/* free page-dir */
	PhysMem::free(pdir->pts.getRoot() >> PAGE_BITS,PhysMem::KERN);
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame = Thread::getRunning()->getFrame();
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAP_AREA),buffer,loadCount);
	return frame;
}

void PageDirBase::zeroToUser(void *dst,size_t count) {
	PageDir *pdir = Proc::getCurPageDir();
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	uintptr_t virt = (uintptr_t)dst;
	while(count > 0) {
		frameno_t frameNo = pdir->getFrameNo(virt);
		size_t amount = MIN(PAGE_SIZE - offset,count);
		uintptr_t addr = ((frameNo << PAGE_BITS) | DIR_MAP_AREA) + offset;
		memclear((void*)addr,amount);
		count -= amount;
		virt += PAGE_SIZE;
		offset = 0;
	}
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
