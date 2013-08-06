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
#include <sys/mem/paging.h>
#include <sys/mem/physmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/virtmem.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/smp.h>
#include <sys/interrupts.h>
#include <sys/boot.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/spinlock.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

/* pte fields */
#define PTE_PRESENT				(1UL << 0)
#define PTE_WRITABLE			(1UL << 1)
#define PTE_NOTSUPER			(1UL << 2)
#define PTE_WRITE_THROUGH		(1UL << 3)
#define PTE_CACHE_DISABLE		(1UL << 4)
#define PTE_ACCESSED			(1UL << 5)
#define PTE_DIRTY				(1UL << 6)
#define PTE_GLOBAL				(1UL << 8)
#define PTE_EXISTS				(1UL << 9)
#define PTE_FRAMENO(pte)		(((pte) >> PAGE_SIZE_SHIFT) & 0xFFFFF)
#define PTE_FRAMENO_MASK		0xFFFFF000

/* pde flags */
#define PDE_PRESENT				(1UL << 0)
#define PDE_WRITABLE			(1UL << 1)
#define PDE_NOTSUPER			(1UL << 2)
#define PDE_WRITE_THROUGH		(1UL << 3)
#define PDE_CACHE_DISABLE		(1UL << 4)
#define PDE_ACCESSED			(1UL << 5)
#define PDE_LARGE				(1UL << 7)
#define PDE_EXISTS				(1UL << 9)
#define PDE_FRAMENO(pde)		(((pde) >> PAGE_SIZE_SHIFT) & 0xFFFFF)
#define PDE_FRAMENO_MASK		0xFFFFF000

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr)	(MAPPED_PTS_START + \
								(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + \
		(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))

#define PAGEDIR(ptables)		((uintptr_t)(ptables) + PAGE_SIZE * (PT_ENTRY_COUNT - 1))

/**
 * Flushes the TLB-entry for the given virtual address.
 * NOTE: supported for >= Intel486
 */
#define	FLUSHADDR(addr)			__asm__ __volatile__ ("invlpg (%0)" : : "r" (addr));

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000)
 * Note that this does not work anymore as soon as the GDT is "corrected" and paging enabled! */
#define VIRT2PHYS(addr)			((uintptr_t)(addr) + 0x40000000)

/* the page-directory for process 0 */
PageDir::pde_t PageDir::proc0PD[PAGE_SIZE / sizeof(pde_t)] A_ALIGNED(PAGE_SIZE);
/* the page-table for process 0 */
PageDir::pte_t PageDir::proc0PT[PAGE_SIZE / sizeof(pte_t)] A_ALIGNED(PAGE_SIZE);
klock_t PageDir::tmpMapLock;
/* TODO we could maintain different locks for userspace and kernelspace; since just the kernel is
 * shared. it would be better to have a global lock for that and a pagedir-lock for the userspace */
klock_t PageDir::lock;
uintptr_t PageDir::freeAreaAddr = FREE_KERNEL_AREA;

void PageDirBase::init() {
	size_t i;
	uintptr_t pd,addr;

	/* note that we assume here that the kernel is not larger than 1 page-table (4MiB)! */

	/* map 1 page-table at 0xC0000000 */
	pd = (uintptr_t)VIRT2PHYS(PageDir::proc0PD);

	/* map one page-table */
	addr = KERNEL_AREA_P_ADDR;
	for(i = 0; i < PT_ENTRY_COUNT; i++, addr += PAGE_SIZE) {
		/* build page-table entry */
		/* we need only 0x7000 (trampoline) and 0xB8000 (vga mem) writable in the first megabyte */
		if(addr < 1 * M && addr != 0x7000 && addr != 0xB8000)
			PageDir::proc0PT[i] = addr | PTE_GLOBAL | PTE_PRESENT | PTE_EXISTS;
		else
			PageDir::proc0PT[i] = addr | PTE_GLOBAL | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
	}

	/* insert page-table in the page-directory */
	PageDir::proc0PD[ADDR_TO_PDINDEX(KERNEL_AREA)] =
			(uintptr_t)VIRT2PHYS(PageDir::proc0PT) | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	PageDir::proc0PD[0] = (uintptr_t)VIRT2PHYS(PageDir::proc0PT) | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

	/* put the page-directory in the last page-dir-slot */
	PageDir::proc0PD[ADDR_TO_PDINDEX(MAPPED_PTS_START)] = pd | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

	/* now set page-dir and enable paging */
	PageDir::activate((uintptr_t)PageDir::proc0PD & ~KERNEL_AREA);
}

void PageDir::activate(uintptr_t pageDir) {
	setPDir(pageDir);
	enable();
	/* enable write-protection; this way, the kernel can't write to readonly-pages */
	/* this helps a lot because we don't have to check in advance whether for copy-on-write and so
	 * on before writing to user-space-memory in kernel */
	setWriteProtection(true);
	/* enable global pages (TODO just possible for >= pentium pro (family 6)) */
	CPU::setCR4(CPU::getCR4() | (1 << 7));
}

ssize_t PageDirBase::mapToCur(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	return Proc::getCurPageDir()->map(virt,frames,count,flags);
}

size_t PageDirBase::unmapFromCur(uintptr_t virt,size_t count,bool freeFrames) {
	return Proc::getCurPageDir()->unmap(virt,count,freeFrames);
}

void PageDirBase::makeFirst() {
	PageDir *pdir = static_cast<PageDir*>(this);
	pdir->own = (uintptr_t)PageDir::proc0PD & ~KERNEL_AREA;
	pdir->other = NULL;
	pdir->lastChange = CPU::rdtsc();
	pdir->otherUpdate = 0;
	pdir->freeKStack = KERNEL_STACK_AREA;
}

void PageDir::mapKernelSpace() {
	uintptr_t addr;
	/* map kernel heap and temp area*/
	for(addr = KERNEL_HEAP_START;
		addr < TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE;
		addr += PAGE_SIZE * PT_ENTRY_COUNT) {
		if(crtPageTable(proc0PD,MAPPED_PTS_START,addr,PG_SUPERVISOR) < 0)
			Util::panic("Not enough kernel-memory for page tables");
	}
	/* map dynamically extending regions */
	for(addr = GFT_AREA; addr < SLLNODE_AREA + SLLNODE_AREA_SIZE; addr += PAGE_SIZE * PT_ENTRY_COUNT) {
		if(crtPageTable(proc0PD,MAPPED_PTS_START,addr,PG_SUPERVISOR) < 0)
			Util::panic("Not enough kernel-memory for page tables");
	}
}

void PageDir::gdtFinished() {
	/* we can simply remove the first 2 page-tables since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0] = 0;
	proc0PD[1] = 0;
	flushTLB();
}

uintptr_t PageDirBase::makeAccessible(uintptr_t phys,size_t pages) {
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t addr = PageDir::freeAreaAddr;
	if(addr + pages * PAGE_SIZE > FREE_KERNEL_AREA + FREE_KERNEL_AREA_SIZE)
		Util::panic("Bootstrap area too small");
	if(phys) {
		size_t i;
		for(i = 0; i < pages; ++i) {
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

uintptr_t PageDir::mapToTemp(const frameno_t *frames,size_t count) {
	assert(frames != NULL && count <= TEMP_MAP_AREA_SIZE / PAGE_SIZE - 1);
	/* the temp-map-area is shared */
	SpinLock::acquire(&tmpMapLock);
	Proc::getCurPageDir()->map(TEMP_MAP_AREA + PAGE_SIZE,frames,count,
			PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	return TEMP_MAP_AREA + PAGE_SIZE;
}

void PageDir::unmapFromTemp(size_t count) {
	Proc::getCurPageDir()->unmap(TEMP_MAP_AREA + PAGE_SIZE,count,false);
	SpinLock::release(&tmpMapLock);
}

int PageDirBase::cloneKernelspace(PageDir *dst,tid_t tid) {
	uintptr_t kstackAddr,kstackPtAddr;
	frameno_t pdirFrame,stackPtFrame;
	PageDir::pde_t *pd,*npd;
	Thread *t = Thread::getById(tid);
	PageDir *cur = Proc::getCurPageDir();
	SpinLock::acquire(&PageDir::lock);

	/* allocate frames */
	pdirFrame = PhysMem::allocate(PhysMem::KERN);
	if(pdirFrame == 0) {
		SpinLock::release(&PageDir::lock);
		return -ENOMEM;
	}
	stackPtFrame = PhysMem::allocate(PhysMem::KERN);
	if(stackPtFrame == 0) {
		PhysMem::free(pdirFrame,PhysMem::KERN);
		SpinLock::release(&PageDir::lock);
		return -ENOMEM;
	}

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	pd = (PageDir::pde_t*)PAGE_DIR_AREA;
	cur->doMap(TEMP_MAP_AREA,&pdirFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	npd = (PageDir::pde_t*)TEMP_MAP_AREA;

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(PageDir::pde_t));
	/* copy kernel-space page-tables */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(ADDR_TO_PDINDEX(KERNEL_STACK_AREA) - ADDR_TO_PDINDEX(KERNEL_AREA)) * sizeof(PageDir::pde_t));
	/* clear the remaining page-tables */
	memclear(npd + ADDR_TO_PDINDEX(KERNEL_STACK_AREA),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_STACK_AREA)) * sizeof(PageDir::pde_t));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)] =
			pdirFrame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	kstackAddr = t->getKernelStack();
	npd[ADDR_TO_PDINDEX(kstackAddr)] =
			stackPtFrame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
	/* clear the page-table */
	kstackPtAddr = ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,kstackAddr);
	FLUSHADDR(kstackPtAddr);
	memclear((void*)kstackPtAddr,PAGE_SIZE);

	cur->doUnmap(TEMP_MAP_AREA,1,false);

	/* one final flush to ensure everything is correct */
	PageDir::flushTLB();
	dst->own = pdirFrame << PAGE_SIZE_SHIFT;
	dst->other = NULL;
	dst->lastChange = CPU::rdtsc();
	dst->otherUpdate = 0;
	if(kstackAddr == KERNEL_STACK_AREA)
		dst->freeKStack = KERNEL_STACK_AREA + PAGE_SIZE;
	else
		dst->freeKStack = KERNEL_STACK_AREA;
	SpinLock::release(&PageDir::lock);
	return 0;
}

uintptr_t PageDir::createKernelStack() {
	uintptr_t addr,end,ptables;
	pte_t *pt;
	pde_t *pd;
	SpinLock::acquire(&lock);
	addr = freeKStack;
	end = KERNEL_STACK_AREA + KERNEL_STACK_AREA_SIZE;
	ptables = getPTables(Proc::getCurPageDir());
	pd = (pde_t*)PAGEDIR(ptables);
	while(addr < end) {
		pt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,addr);
		/* use this address if either the page-table or the page doesn't exist yet */
		if(!(pd[ADDR_TO_PDINDEX(addr)] & PDE_EXISTS) || !(*pt & PTE_EXISTS)) {
			if(doMap(addr,NULL,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
				addr = 0;
			break;
		}
		addr += PAGE_SIZE;
	}
	if(addr == end)
		addr = 0;
	else
		freeKStack = addr + PAGE_SIZE;
	SpinLock::release(&lock);
	return addr;
}

void PageDir::removeKernelStack(uintptr_t addr) {
	SpinLock::acquire(&lock);
	if(addr < freeKStack)
		freeKStack = addr;
	doUnmap(addr,1,true);
	SpinLock::release(&lock);
}

void PageDirBase::destroy() {
	PageDir *pdir = static_cast<PageDir*>(this);
	size_t i;
	uintptr_t ptables;
	PageDir::pde_t *pd;
	SpinLock::acquire(&PageDir::lock);
	ptables = pdir->getPTables(Proc::getCurPageDir());
	pd = (PageDir::pde_t*)PAGEDIR(ptables);
	/* free page-tables for kernel-stack */
	for(i = ADDR_TO_PDINDEX(KERNEL_STACK_AREA);
		i < ADDR_TO_PDINDEX(KERNEL_STACK_AREA + KERNEL_STACK_AREA_SIZE); i++) {
		if(pd[i] & PDE_EXISTS) {
			PhysMem::free(PDE_FRAMENO(pd[i]),PhysMem::KERN);
			pd[i] = 0;
		}
	}
	/* free page-dir */
	PhysMem::free(pdir->own >> PAGE_SIZE_SHIFT,PhysMem::KERN);
	SpinLock::release(&PageDir::lock);
}

bool PageDirBase::isPresent(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t ptables;
	PageDir::pde_t *pd;
	bool res = false;
	SpinLock::acquire(&PageDir::lock);
	ptables = pdir->getPTables(Proc::getCurPageDir());
	pd = (PageDir::pde_t*)PAGEDIR(ptables);
	if((pd[ADDR_TO_PDINDEX(virt)] & (PDE_PRESENT | PDE_EXISTS)) == (PDE_PRESENT | PDE_EXISTS)) {
		PageDir::pte_t *pt = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		res = (*pt & (PTE_PRESENT | PTE_EXISTS)) == (PTE_PRESENT | PTE_EXISTS);
	}
	SpinLock::release(&PageDir::lock);
	return res;
}

frameno_t PageDirBase::getFrameNo(uintptr_t virt) const {
	const PageDir *pdir = static_cast<const PageDir*>(this);
	uintptr_t ptables;
	frameno_t res = -1;
	PageDir::pte_t *pt;
	PageDir::pde_t *pde;
	SpinLock::acquire(&PageDir::lock);
	ptables = pdir->getPTables(Proc::getCurPageDir());
	pde = (PageDir::pde_t*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(virt);
	pt = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	res = PTE_FRAMENO(*pt);
	SpinLock::release(&PageDir::lock);
	assert((*pde & (PDE_PRESENT | PDE_EXISTS)) == (PDE_PRESENT | PDE_EXISTS));
	assert((*pt & (PTE_PRESENT | PTE_EXISTS)) == (PTE_PRESENT | PTE_EXISTS));
	return res;
}

frameno_t PageDirBase::demandLoad(const void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame;
	PageDir *pdir = Proc::getCurPageDir();
	SpinLock::acquire(&PageDir::lock);
	frame = Thread::getRunning()->getFrame();
	pdir->doMap(TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,buffer,loadCount);
	pdir->doUnmap(TEMP_MAP_AREA,1,false);
	SpinLock::release(&PageDir::lock);
	return frame;
}

void PageDirBase::copyToFrame(frameno_t frame,const void *src) {
	PageDir *pdir = Proc::getCurPageDir();
	SpinLock::acquire(&PageDir::lock);
	pdir->doMap(TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,src,PAGE_SIZE);
	pdir->doUnmap(TEMP_MAP_AREA,1,false);
	SpinLock::release(&PageDir::lock);
}

void PageDirBase::copyFromFrame(frameno_t frame,void *dst) {
	PageDir *pdir = Proc::getCurPageDir();
	SpinLock::acquire(&PageDir::lock);
	pdir->doMap(TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy(dst,(void*)TEMP_MAP_AREA,PAGE_SIZE);
	pdir->doUnmap(TEMP_MAP_AREA,1,false);
	SpinLock::release(&PageDir::lock);
}

ssize_t PageDirBase::clonePages(PageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,size_t count,
                                bool share) {
	PageDir *src = static_cast<PageDir*>(this);
	PageDir *cur = Proc::getCurPageDir();
	ssize_t pts = 0;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	uintptr_t srctables,dsttables;
	PageDir::pde_t *dstpdir;
	assert(this != dst && (this == cur || dst == cur));
	SpinLock::acquire(&PageDir::lock);
	srctables = src->getPTables(cur);
	dsttables = dst->getPTables(cur);
	dstpdir = (PageDir::pde_t*)PAGEDIR(dsttables);
	while(count > 0) {
		PageDir::pte_t *dpt;
		PageDir::pte_t *spt = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(srctables,virtSrc);
		PageDir::pte_t pte = *spt;
		/* page table not present? */
		if(!(dstpdir[ADDR_TO_PDINDEX(virtDst)] & PDE_PRESENT)) {
			if(PageDir::crtPageTable(dstpdir,dsttables,virtDst,pte) < 0)
				goto error;
			pts++;
		}
		dpt = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(dsttables,virtDst);

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_PRESENT)))
			pte &= ~PTE_WRITABLE;
		*dpt = pte;
		if(dst == cur)
			FLUSHADDR(virtDst);

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_PRESENT)) {
			*spt &= ~PTE_WRITABLE;
			if(this == cur)
				FLUSHADDR(virtSrc);
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	SMP::flushTLB(src);
	SpinLock::release(&PageDir::lock);
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	dst->unmap(orgVirtDst,orgCount - count,false);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		PageDir::pte_t *pte = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(srctables,orgVirtSrc);
		if(!share && (*pte & PTE_PRESENT))
			src->doMap(orgVirtSrc,NULL,1,PG_PRESENT | PG_WRITABLE | PG_KEEPFRM);
		orgVirtSrc += PAGE_SIZE;
		orgCount--;
	}
	SpinLock::release(&PageDir::lock);
	return -ENOMEM;
}

int PageDir::crtPageTable(PageDir::pde_t *pd,uintptr_t ptables,uintptr_t virt,uint flags) {
	frameno_t frame;
	size_t pdi = ADDR_TO_PDINDEX(virt);
	assert((pd[pdi] & PDE_PRESENT) == 0);
	/* get new frame for page-table */
	frame = PhysMem::allocate(PhysMem::KERN);
	if(frame == 0)
		return -ENOMEM;
	/* writable because we want to be able to change PTE's in the PTE-area */
	/* is there another reason? :) */
	pd[pdi] = frame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
	if(!(flags & PG_SUPERVISOR))
		pd[pdi] |= PDE_NOTSUPER;

	flushPageTable(virt,ptables);
	/* clear frame (ensure that we start at the beginning of the frame) */
	memclear((void*)ADDR_TO_MAPPED_CUSTOM(ptables,
			virt & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),PAGE_SIZE);
	return 0;
}

ssize_t PageDir::doMap(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	ssize_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t ptables = getPTables(cur);
	pde_t *pdirAddr = (pde_t*)PAGEDIR(ptables);
	bool freeFrames;
	pte_t *ptep,pte,pteFlags = PTE_EXISTS;

	virt &= ~(PAGE_SIZE - 1);
	if(flags & PG_PRESENT)
		pteFlags |= PTE_PRESENT;
	if((flags & PG_PRESENT) && (flags & PG_WRITABLE))
		pteFlags |= PTE_WRITABLE;
	if(!(flags & PG_SUPERVISOR))
		pteFlags |= PTE_NOTSUPER;
	if(flags & PG_GLOBAL)
		pteFlags |= PTE_GLOBAL;

	while(count > 0) {
		/* create page-table if necessary */
		if(!(pdirAddr[ADDR_TO_PDINDEX(virt)] & PDE_PRESENT)) {
			if(crtPageTable(pdirAddr,ptables,virt,flags) < 0)
				goto error;
			pts++;
		}

		/* setup page */
		ptep = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		pte = pteFlags;
		if(flags & PG_KEEPFRM)
			pte |= *ptep & PTE_FRAMENO_MASK;
		else if(flags & PG_PRESENT) {
			/* get frame */
			if(frames == NULL) {
				frameno_t frame;
				if(virt >= KERNEL_AREA) {
					frame = PhysMem::allocate(virt < KERNEL_STACK_AREA ? PhysMem::CRIT : PhysMem::KERN);
					if(frame == 0)
						goto error;
				}
				else
					frame = Thread::getRunning()->getFrame();
				pte |= frame << PAGE_SIZE_SHIFT;
			}
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pte |= *frames++ & PTE_FRAMENO_MASK;
				else
					pte |= *frames++ << PAGE_SIZE_SHIFT;
			}
		}
		*ptep = pte;

		/* invalidate TLB-entry */
		if(this == cur)
			FLUSHADDR(virt);

		/* to next page */
		virt += PAGE_SIZE;
		count--;
	}

	lastChange = CPU::rdtsc();
	SMP::flushTLB(this);
	return pts;

error:
	freeFrames = frames == NULL && !(flags & PG_KEEPFRM) && (flags & PG_PRESENT);
	doUnmap(orgVirt,orgCount - count,freeFrames);
	return -ENOMEM;
}

size_t PageDir::doUnmap(uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts = 0;
	PageDir *cur = Proc::getCurPageDir();
	uintptr_t ptables = getPTables(cur);
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	pte_t *pte = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				pts += remEmptyPt(ptables,lastPti);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		if(*pte & PTE_PRESENT) {
			*pte &= ~PTE_PRESENT;
			if(freeFrames) {
				frameno_t frame = PTE_FRAMENO(*pte);
				if(virt >= KERNEL_AREA) {
					if(virt < KERNEL_STACK_AREA)
						PhysMem::free(frame,PhysMem::CRIT);
					else
						PhysMem::free(frame,PhysMem::KERN);
				}
				else
					PhysMem::free(frame,PhysMem::USR);
			}
		}
		*pte &= ~PTE_EXISTS;

		/* invalidate TLB-entry */
		if(this == cur)
			FLUSHADDR(virt);

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
		pts += remEmptyPt(ptables,pti);
	lastChange = CPU::rdtsc();
	SMP::flushTLB(this);
	return pts;
}

uintptr_t PageDir::getPTables(PageDir *cur) const {
	pde_t *pde;
	if(this == cur)
		return MAPPED_PTS_START;
	if(this == cur->other) {
		/* do we have to refresh the mapping? */
		if(lastChange <= cur->otherUpdate)
			return TMPMAP_PTS_START;
	}
	/* map page-tables to other area */
	pde = ((pde_t*)PAGE_DIR_AREA) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	*pde = own | PDE_PRESENT | PDE_EXISTS | PDE_WRITABLE;
	/* we want to access the whole page-table */
	flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	cur->other = other;
	cur->otherUpdate = CPU::rdtsc();
	return TMPMAP_PTS_START;
}

size_t PageDir::remEmptyPt(uintptr_t ptables,size_t pti) {
	size_t i;
	pde_t *pde;
	uintptr_t virt = pti * PAGE_SIZE * PT_ENTRY_COUNT;
	pte_t *pte = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pte[i] & PTE_EXISTS)
			return 0;
	}
	/* empty => can be removed */
	pde = (pde_t*)PAGEDIR(ptables) + pti;
	PhysMem::free(PDE_FRAMENO(*pde),PhysMem::KERN);
	*pde = 0;
	if(ptables == MAPPED_PTS_START)
		flushPageTable(virt,ptables);
	else
		FLUSHADDR((uintptr_t)pte);
	return 1;
}

size_t PageDirBase::getPTableCount() const {
	size_t i,count = 0;
	uintptr_t ptables;
	PageDir::pde_t *pdirAddr;
	SpinLock::acquire(&PageDir::lock);
	ptables = static_cast<const PageDir*>(this)->getPTables(Proc::getCurPageDir());
	pdirAddr = (PageDir::pde_t*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdirAddr[i] & PDE_PRESENT)
			count++;
	}
	SpinLock::release(&PageDir::lock);
	return count;
}

size_t PageDirBase::getPageCount() const {
	size_t i,x,count = 0;
	PageDir::pde_t *pdir = (PageDir::pde_t*)PAGE_DIR_AREA;
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdir[i] & PDE_PRESENT) {
			PageDir::pte_t *pt = (PageDir::pte_t*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pt[x] & PTE_PRESENT)
					count++;
			}
		}
	}
	return count;
}

void PageDirBase::print(OStream &os,uint parts) const {
	size_t i;
	PageDir *cur = Proc::getCurPageDir();
	PageDir *old = cur->other;
	uintptr_t ptables;
	PageDir::pde_t *pdirAddr;
	SpinLock::acquire(&PageDir::lock);
	ptables = static_cast<const PageDir*>(this)->getPTables(cur);
	/* note that we release the lock here because os.writef() might cause the cache-module to
	 * allocate more memory and use paging to map it. therefore, we would cause a deadlock if
	 * we use don't release it here.
	 * I think, in this case we can do it without lock because the only things that could happen
	 * are a delivery of wrong information to the user and a segfault, which would kill the
	 * process. Of course, only the process that is reading the information could cause these
	 * problems. Therefore, its not really a bad thing. */
	SpinLock::release(&PageDir::lock);
	pdirAddr = (PageDir::pde_t*)PAGEDIR(ptables);
	os.writef("page-dir @ %p:\n",pdirAddr);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(!(pdirAddr[i] & PDE_PRESENT))
			continue;
		if(parts == PD_PART_ALL ||
			(i < ADDR_TO_PDINDEX(KERNEL_AREA) && (parts & PD_PART_USER)) ||
			(i >= ADDR_TO_PDINDEX(KERNEL_AREA) &&
					i < ADDR_TO_PDINDEX(KERNEL_HEAP_START) &&
					(parts & PD_PART_KERNEL)) ||
			(i >= ADDR_TO_PDINDEX(TEMP_MAP_AREA) &&
					i < ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE) &&
					(parts & PD_PART_TEMPMAP)) ||
			(i >= ADDR_TO_PDINDEX(KERNEL_HEAP_START) &&
					i < ADDR_TO_PDINDEX(KERNEL_HEAP_START + KERNEL_HEAP_SIZE) &&
					(parts & PD_PART_KHEAP)) ||
			(i >= ADDR_TO_PDINDEX(MAPPED_PTS_START) && (parts & PD_PART_PTBLS))) {
			PageDir::printPageTable(os,ptables,i,pdirAddr[i]);
		}
	}
	os.writef("\n");
	if(cur->other != old)
		old->getPTables(cur);
}

void PageDirBase::printPage(OStream &os,uintptr_t virt) const {
	/* don't use locking here; its only used in the debugging-console anyway. its better without
	 * locking to be able to view these things even if somebody currently holds the lock (e.g.
	 * because a panic occurred during that operation) */
	PageDir *cur = Proc::getCurPageDir();
	PageDir *old = cur->other;
	uintptr_t ptables;
	PageDir::pde_t *pdirAddr;
	ptables = static_cast<const PageDir*>(this)->getPTables(cur);
	pdirAddr = (PageDir::pde_t*)PAGEDIR(ptables);
	if(pdirAddr[ADDR_TO_PDINDEX(virt)] & PDE_PRESENT) {
		PageDir::pte_t *page = (PageDir::pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		os.writef("Page @ %p: ",virt);
		printPage(os,*page);
		os.writef("\n");
	}
	/* restore the old mapping, if necessary */
	if(cur->other != old)
		old->getPTables(cur);
}

void PageDir::printPageTable(OStream &os,uintptr_t ptables,size_t no,pde_t pde) {
	size_t i;
	uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	pte_t *pte = (pte_t*)(ptables + no * PAGE_SIZE);
	os.writef("\tpt 0x%x [frame 0x%x, %c%c] @ %p: (VM: %p - %p)\n",no,
			PDE_FRAMENO(pde),(pde & PDE_NOTSUPER) ? 'u' : 'k',
			(pde & PDE_WRITABLE) ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & PTE_EXISTS) {
				os.writef("\t\t0x%zx: ",i);
				printPage(os,pte[i]);
				os.writef(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

void PageDir::printPage(OStream &os,pte_t page) {
	if(page & PTE_EXISTS) {
		os.writef("r=0x%08x fr=0x%x [%c%c%c%c]",page,PTE_FRAMENO(page),
				(page & PTE_PRESENT) ? 'p' : '-',(page & PTE_NOTSUPER) ? 'u' : 'k',
				(page & PTE_WRITABLE) ? 'w' : 'r',(page & PTE_GLOBAL) ? 'g' : '-');
	}
	else {
		os.writef("-");
	}
}
