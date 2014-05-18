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
#include <assert.h>

extern void *proc0TLPD;
/* we can't allocate any frames at the beginning. so put the shared-pagetables in bss */
PageDir::pte_t PageDir::sharedPtbls[(DIR_MAP_AREA - KHEAP_START) / PT_SIZE][PAGE_SIZE] A_ALIGNED(PAGE_SIZE);

void PageDirBase::init() {
	PageDir::pte_t *pt = (PageDir::pte_t*)&proc0TLPD;

	/* map shared page-tables */
	uint flags = PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL | PTE_EXISTS;
	uintptr_t virt = KHEAP_START;
	size_t start = ADDR_TO_PDINDEX(virt);
	for(size_t i = start; virt < DIR_MAP_AREA; ++i) {
		pt[i] = ((uintptr_t)PageDir::sharedPtbls[i - start] - KERNEL_AREA) | flags;
		virt += PT_SIZE;
	}

	/* map first part of physical memory contiguously */
	virt = DIR_MAP_AREA;
	flags = PTE_PRESENT | PTE_WRITABLE | PTE_LARGE | PTE_EXISTS;
	size_t end = ADDR_TO_PDINDEX(DIR_MAP_AREA + DIR_MAP_AREA_SIZE);
	for(size_t i = ADDR_TO_PDINDEX(virt); i < end; ++i) {
		pt[i] = (virt - DIR_MAP_AREA) | flags;
		virt += PT_SIZE;
	}

	/* enable write-protection; this way, the kernel can't write to readonly-pages */
	/* this helps a lot because we don't have to check in advance for copy-on-write and so
	 * on before writing to user-space-memory in kernel */
	PageDir::setWriteProtection(true);
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

	const PageDir::pte_t *pd = (const PageDir::pte_t*)(DIR_MAP_AREA + cur->pagedir);
	PageDir::pte_t *npd = (PageDir::pte_t*)(DIR_MAP_AREA + (pdirFrame << PAGE_BITS));

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(PageDir::pte_t));
	/* copy kernel-space page-tables */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(ADDR_TO_PDINDEX(KSTACK_AREA) - ADDR_TO_PDINDEX(KERNEL_AREA)) * sizeof(PageDir::pte_t));
	/* clear the remaining page-tables */
	memclear(npd + ADDR_TO_PDINDEX(KSTACK_AREA),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KSTACK_AREA)) * sizeof(PageDir::pte_t));

	/* get new page-table for the kernel-stack-area and the stack itself */
	uintptr_t kstackAddr = t->getKernelStack();
	npd[ADDR_TO_PDINDEX(kstackAddr)] =
			stackPtFrame << PAGE_BITS | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	/* clear the page-table */
	memclear((void*)(DIR_MAP_AREA + (stackPtFrame << PAGE_BITS)),PAGE_SIZE);

	/* one final flush to ensure everything is correct */
	PageDir::flushTLB();
	dst->pagedir = pdirFrame << PAGE_BITS;
	if(kstackAddr == KSTACK_AREA)
		dst->freeKStack = KSTACK_AREA + PAGE_SIZE;
	else
		dst->freeKStack = KSTACK_AREA;
	dst->lock = SpinLock();
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	PageDir::pte_t *pd = (PageDir::pte_t*)(DIR_MAP_AREA + pdir->pagedir);
	/* free page-tables for kernel-stack */
	for(size_t i = ADDR_TO_PDINDEX(KSTACK_AREA);
		i < ADDR_TO_PDINDEX(KSTACK_AREA + KSTACK_AREA_SIZE - 1); i++) {
		if(pd[i] & PTE_EXISTS) {
			PhysMem::free(PTE_FRAMENO(pd[i]),PhysMem::KERN);
			pd[i] = 0;
		}
	}
	/* free page-dir */
	PhysMem::free(pdir->pagedir >> PAGE_BITS,PhysMem::KERN);
}
