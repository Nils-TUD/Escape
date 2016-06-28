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
#include <task/proc.h>
#include <assert.h>
#include <common.h>

#define SHPT_COUNT		((DIR_MAP_AREA - KHEAP_START) / PT_SIZE)

extern void *proc0TLPD;
/* we can't allocate any frames at the beginning. so put the shared-pagetables in bss */
uint8_t PageDir::sharedPtbls[SHPT_COUNT][PAGE_SIZE] A_ALIGNED(PAGE_SIZE);

void PageDirBase::init() {
	/* map first part of physical memory contiguously */
	pte_t *pt = (pte_t*)&proc0TLPD;
	uintptr_t virt = DIR_MAP_AREA;
	uint flags = PTE_PRESENT | PTE_WRITABLE | PTE_LARGE | PTE_EXISTS;
	size_t end = PageTables::index(DIR_MAP_AREA + DIR_MAP_AREA_SIZE,1);
	for(size_t i = PageTables::index(virt,1); i < end; ++i) {
		pt[i] = (virt - DIR_MAP_AREA) | flags;
		virt += PT_SIZE;
	}

	/* create temp object for the start */
	PageDir pdir;
	pdir.pts.setRoot((pte_t)&proc0TLPD & ~KERNEL_AREA);
	pdir.freeKStack = KSTACK_AREA;
	pdir.lock = SpinLock();

	/* map shared page-tables */
	flags = PG_NOPAGES | PG_SUPERVISOR;
	PageDir::PTAllocator alloc((uintptr_t)PageDir::sharedPtbls & ~KERNEL_AREA,SHPT_COUNT);
	pdir.map(KHEAP_START,(DIR_MAP_AREA - KHEAP_START) / PAGE_SIZE,alloc,flags);

	/* enable write-protection; this way, the kernel can't write to readonly-pages */
	/* this helps a lot because we don't have to check in advance for copy-on-write and so
	 * on before writing to user-space-memory in kernel */
	PageDir::setWriteProtection(true);
}

void PageDir::enableNXE() {
}

int PageDirBase::cloneKernelspace(PageDir *dst,tid_t tid) {
	Thread *t = Thread::getById(tid);
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

	const pte_t *pd = (const pte_t*)(DIR_MAP_AREA + cur->pts.getRoot());
	pte_t *npd = (pte_t*)(DIR_MAP_AREA + (pdirFrame << PAGE_BITS));

	/* clear user-space page-tables */
	memclear(npd,PageTables::index(KERNEL_AREA,1) * sizeof(pte_t));
	/* copy kernel-space page-tables */
	memcpy(npd + PageTables::index(KERNEL_AREA,1),
			pd + PageTables::index(KERNEL_AREA,1),
			(PageTables::index(KSTACK_AREA,1) - PageTables::index(KERNEL_AREA,1)) * sizeof(pte_t));
	/* clear the remaining page-tables */
	memclear(npd + PageTables::index(KSTACK_AREA,1),
			(PT_ENTRY_COUNT - PageTables::index(KSTACK_AREA,1)) * sizeof(pte_t));

	/* get new page-table for the kernel-stack-area and the stack itself */
	uintptr_t kstackAddr = t->getKernelStack();
	npd[PageTables::index(kstackAddr,1)] =
			stackPtFrame << PAGE_BITS | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	/* clear the page-table */
	memclear((void*)(DIR_MAP_AREA + (stackPtFrame << PAGE_BITS)),PAGE_SIZE);

	/* one final flush to ensure everything is correct */
	PageDir::flushTLB();
	dst->pts.setRoot(pdirFrame << PAGE_BITS);
	if(kstackAddr == KSTACK_AREA)
		dst->freeKStack = KSTACK_AREA + PAGE_SIZE;
	else
		dst->freeKStack = KSTACK_AREA;
	dst->lock = SpinLock();
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	pte_t *pd = (pte_t*)(DIR_MAP_AREA + pdir->pts.getRoot());
	/* free page-tables for kernel-stack */
	for(size_t i = PageTables::index(KSTACK_AREA,1);
		i < PageTables::index(KSTACK_AREA + KSTACK_AREA_SIZE - 1,1); i++) {
		if(pd[i] & PTE_EXISTS) {
			PhysMem::free(PTE_FRAMENO(pd[i]),PhysMem::KERN);
			pd[i] = 0;
		}
	}
	/* free page-dir */
	PhysMem::free(pdir->pts.getRoot() >> PAGE_BITS,PhysMem::KERN);
}
