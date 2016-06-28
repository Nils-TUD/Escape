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

#define SHARED_AREA_SIZE	(KFREE_AREA + KFREE_AREA_SIZE - KHEAP_START)
#define SHPT_COUNT			(1 + (DIR_MAP_AREA_SIZE / PD_SIZE) + \
 							((SHARED_AREA_SIZE + PDPT_SIZE - 1) / PDPT_SIZE) + \
 							((SHARED_AREA_SIZE + PD_SIZE - 1) / PD_SIZE) + \
 							((SHARED_AREA_SIZE + PT_SIZE - 1) / PT_SIZE))

extern void *proc0TLPD;
/* we can't allocate any frames at the beginning. so put the shared-pagetables in bss */
uint8_t PageDir::sharedPtbls[SHPT_COUNT][PAGE_SIZE] A_ALIGNED(PAGE_SIZE);

void PageDirBase::init() {
	size_t shpt = 0;

	PageDir::enableNXE();

	/* put entry for physical memory in PML4 */
	pte_t *pt2,*pt = (pte_t*)&proc0TLPD;
	uintptr_t ptAddr = (uintptr_t)PageDir::sharedPtbls[shpt++] & ~KERNEL_BEGIN;
	pt[PageTables::index(DIR_MAP_AREA,3)] = ptAddr | PTE_EXISTS | PTE_PRESENT | PTE_WRITABLE;

	/* map first part of physical memory contiguously */
	uintptr_t addr = 0;
	pt = (pte_t*)(ptAddr | KERNEL_BEGIN);
	for(size_t i = 0; i < DIR_MAP_AREA_SIZE / PD_SIZE; ++i) {
		ptAddr = (uintptr_t)PageDir::sharedPtbls[shpt++] & ~KERNEL_BEGIN;
		pt[i] = ptAddr | PTE_EXISTS | PTE_PRESENT | PTE_WRITABLE;
		pt2 = (pte_t*)(ptAddr | KERNEL_BEGIN);
		for(size_t j = 0; j < PT_ENTRY_COUNT; ++j) {
			pt2[j] = addr | PTE_EXISTS | PTE_LARGE | PTE_PRESENT | PTE_WRITABLE;
			addr += PT_SIZE;
		}
	}

	/* create temp object for the start */
	PageDir pdir;
	pdir.pts.setRoot((pte_t)&proc0TLPD);
	pdir.freeKStack = KSTACK_AREA;
	pdir.lock = SpinLock();

	/* map shared page-tables */
	uint flags = PG_NOPAGES | PG_SUPERVISOR;
	PageDir::PTAllocator alloc((uintptr_t)PageDir::sharedPtbls[shpt] & ~KERNEL_BEGIN,SHPT_COUNT - shpt);
	pdir.map(KHEAP_START,SHARED_AREA_SIZE / PAGE_SIZE,alloc,flags);

	/* enable write-protection; this way, the kernel can't write to readonly-pages */
	/* this helps a lot because we don't have to check in advance for copy-on-write and so
	 * on before writing to user-space-memory in kernel */
	PageDir::setWriteProtection(true);
}

void PageDir::enableNXE() {
	// enable NXE only if supported
	if(CPU::hasFeature(CPU::INTEL,CPU::FEAT_NX)) {
		CPU::setMSR(CPU::MSR_EFER,CPU::getMSR(CPU::MSR_EFER) | CPU::EFER_NXE);
		PageTables::enableNXE();
	}
}

int PageDirBase::cloneKernelspace(PageDir *dst,tid_t tid) {
	Thread *t = Thread::getById(tid);
	PageDir *cur = Proc::getCurPageDir();

	frameno_t pml4Frame = PhysMem::allocate(PhysMem::KERN);
	if(pml4Frame == PhysMem::INVALID_FRAME)
		return -ENOMEM;

	dst->pts.setRoot(pml4Frame << PAGE_BITS);
	if(t->getKernelStack() == KSTACK_AREA)
		dst->freeKStack = KSTACK_AREA + PAGE_SIZE;
	else
		dst->freeKStack = KSTACK_AREA;
	dst->lock = SpinLock();

	const pte_t *pml4 = (const pte_t*)(DIR_MAP_AREA + cur->pts.getRoot());
	pte_t *npml4 = (pte_t*)(DIR_MAP_AREA + (pml4Frame << PAGE_BITS));

	/* clear user-space PML4 entries */
	memclear(npml4,PageTables::index(KERNEL_AREA,3) * sizeof(pte_t));
	/* copy remaining PML4 entries */
	memcpy(npml4 + PageTables::index(KERNEL_AREA,3),pml4 + PageTables::index(KERNEL_AREA,3),
			(PT_ENTRY_COUNT - PageTables::index(KERNEL_AREA,3)) * sizeof(pte_t));

	/* clear entry for non-shared page-tables */
	npml4[PageTables::index(KSTACK_AREA,3)] = 0;

	/* map kernel-stack */
	PageTables::KStackAllocator alloc;
	dst->map(t->getKernelStack(),1,alloc,PG_NOPAGES | PG_SUPERVISOR);
	return 0;
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	/* unmap kernel-stacks */
	PageTables::KStackAllocator alloc;
	pdir->unmap(KSTACK_AREA,KSTACK_AREA_SIZE / PAGE_SIZE,alloc);
	/* free page-dir */
	PhysMem::free(pdir->pts.getRoot() >> PAGE_BITS,PhysMem::KERN);
}
