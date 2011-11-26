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
#include <sys/mem/pmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/smp.h>
#include <sys/intrpt.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/printf.h>
#include <sys/spinlock.h>
#include <esc/sllist.h>
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

typedef uint32_t pte_t;
typedef uint32_t pde_t;

/**
 * Assembler routine to enable paging
 */
extern void paging_enable(void);

/**
 * Assembler routine to flush the TLB
 */
extern void paging_flushTLB(void);

/**
 * Flushes the whole page-table including the page in the mapped page-table-area
 *
 * @param virt a virtual address in the page-table
 * @param ptables the page-table mapping-area
 */
static void paging_flushPageTable(uintptr_t virt,uintptr_t ptables);

static void paging_setWriteProtection(bool enabled);
static int paging_crtPageTable(pde_t *pd,uintptr_t ptables,uintptr_t virt,uint flags);
static ssize_t paging_doMapTo(pagedir_t *pdir,uintptr_t virt,const frameno_t *frames,
		size_t count,uint flags);
static size_t paging_doUnmapFrom(pagedir_t *pdir,uintptr_t virt,size_t count,bool freeFrames);
static pagedir_t *paging_getPageDir(void);
static uintptr_t paging_getPTables(pagedir_t *cur,pagedir_t *pdir);
static size_t paging_remEmptyPt(uintptr_t ptables,size_t pti);

static void paging_printPageTable(uintptr_t ptables,size_t no,pde_t pde) ;
static void paging_printPage(pte_t page);

/* the page-directory for process 0 */
static pde_t proc0PD[PAGE_SIZE / sizeof(pde_t)] A_ALIGNED(PAGE_SIZE);
/* the page-tables for process 0 (two because our mm-stack will get large if we have a lot physmem) */
static pte_t proc0PT1[PAGE_SIZE / sizeof(pte_t)] A_ALIGNED(PAGE_SIZE);
static pte_t proc0PT2[PAGE_SIZE / sizeof(pte_t)] A_ALIGNED(PAGE_SIZE);
static klock_t tmpMapLock;
/* TODO we could maintain different locks for userspace and kernelspace; since just the kernel is
 * shared. it would be better to have a global lock for that and a pagedir-lock for the userspace */
static klock_t pagingLock;

void paging_init(void) {
	size_t j,i;
	uintptr_t pd,vaddr,addr,end;
	pte_t *pts[] = {proc0PT1,proc0PT2};

	/* note that we assume here that the kernel including mm-stack is not larger than 2
	 * complete page-tables (8MiB)! */

	/* map 2 page-tables at 0xC0000000 */
	vaddr = KERNEL_AREA;
	addr = KERNEL_AREA_P_ADDR;
	pd = (uintptr_t)VIRT2PHYS(proc0PD);
	for(j = 0; j < 2; j++) {
		uintptr_t pt = VIRT2PHYS(pts[j]);

		/* map one page-table */
		end = addr + (PT_ENTRY_COUNT) * PAGE_SIZE;
		for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
			/* build page-table entry */
			pts[j][i] = addr | PTE_GLOBAL | PTE_PRESENT | PTE_WRITABLE | PTE_EXISTS;
		}

		/* insert page-table in the page-directory */
		proc0PD[ADDR_TO_PDINDEX(vaddr)] = pt | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
		/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
		proc0PD[j] = pt | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

		vaddr += PT_ENTRY_COUNT * PAGE_SIZE;
	}

	/* put the page-directory in the last page-dir-slot */
	proc0PD[ADDR_TO_PDINDEX(MAPPED_PTS_START)] = pd | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

	/* now set page-dir and enable paging */
	paging_activate((uintptr_t)proc0PD & ~KERNEL_AREA);
}

void paging_activate(uintptr_t pageDir) {
	paging_exchangePDir(pageDir);
	paging_enable();
	/* enable write-protection; this way, the kernel can't write to readonly-pages */
	/* this helps a lot because we don't have to check in advance whether for copy-on-write and so
	 * on before writing to user-space-memory in kernel */
	paging_setWriteProtection(true);
	/* enable global pages (TODO just possible for >= pentium pro (family 6)) */
	cpu_setCR4(cpu_getCR4() | (1 << 7));
}

static void paging_setWriteProtection(bool enabled) {
	if(enabled)
		cpu_setCR0(cpu_getCR0() | CR0_WRITE_PROTECT);
	else
		cpu_setCR0(cpu_getCR0() & ~CR0_WRITE_PROTECT);
}

void paging_setFirst(pagedir_t *pdir) {
	pdir->own = (uintptr_t)proc0PD & ~KERNEL_AREA;
	pdir->other = NULL;
	pdir->lastChange = cpu_rdtsc();
	pdir->otherUpdate = 0;
	pdir->freeKStack = KERNEL_STACK_AREA;
}

void paging_mapKernelSpace(void) {
	uintptr_t addr,end;
	pde_t *pde;
	/* insert all page-tables for 0xC0800000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_AREA + (PAGE_SIZE * PT_ENTRY_COUNT * 2);
	end = KERNEL_STACK_AREA;
	pde = (pde_t*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		frameno_t frame = pmem_allocate(FRM_KERNEL);
		if(frame == 0)
			util_panic("Not enough kernel-memory");
		*pde = frame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
		/* clear */
		memclear((void*)ADDR_TO_MAPPED(addr),PAGE_SIZE);
		/* to next */
		pde++;
		addr += PAGE_SIZE * PT_ENTRY_COUNT;
	}
}

void paging_gdtFinished(void) {
	/* we can simply remove the first 2 page-tables since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0] = 0;
	proc0PD[1] = 0;
	paging_flushTLB();
}

bool paging_isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= KERNEL_AREA && virt + count >= virt;
}

uintptr_t paging_mapToTemp(const frameno_t *frames,size_t count) {
	assert(frames != NULL && count <= TEMP_MAP_AREA_SIZE / PAGE_SIZE - 1);
	/* the temp-map-area is shared */
	spinlock_aquire(&tmpMapLock);
	paging_map(TEMP_MAP_AREA + PAGE_SIZE,frames,count,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	return TEMP_MAP_AREA + PAGE_SIZE;
}

void paging_unmapFromTemp(size_t count) {
	paging_unmap(TEMP_MAP_AREA + PAGE_SIZE,count,false);
	spinlock_release(&tmpMapLock);
}

int paging_cloneKernelspace(pagedir_t *pdir,uintptr_t kstackAddr) {
	uintptr_t kstackPtAddr;
	frameno_t pdirFrame,stackPtFrame;
	pde_t *pd,*npd;
	pagedir_t *cur = paging_getPageDir();
	spinlock_aquire(&pagingLock);

	/* allocate frames */
	pdirFrame = pmem_allocate(FRM_KERNEL);
	if(pdirFrame == 0) {
		spinlock_release(&pagingLock);
		return -ENOMEM;
	}
	stackPtFrame = pmem_allocate(FRM_KERNEL);
	if(stackPtFrame == 0) {
		pmem_free(pdirFrame,FRM_KERNEL);
		spinlock_release(&pagingLock);
		return -ENOMEM;
	}

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	pd = (pde_t*)PAGE_DIR_AREA;
	paging_doMapTo(cur,TEMP_MAP_AREA,&pdirFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	npd = (pde_t*)TEMP_MAP_AREA;

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(pde_t));
	/* copy kernel-space page-tables */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(ADDR_TO_PDINDEX(KERNEL_STACK_AREA) - ADDR_TO_PDINDEX(KERNEL_AREA)) * sizeof(pde_t));
	/* clear the remaining page-tables */
	memclear(npd + ADDR_TO_PDINDEX(KERNEL_STACK_AREA),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_STACK_AREA)) * sizeof(pde_t));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)] =
			pdirFrame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	npd[ADDR_TO_PDINDEX(kstackAddr)] =
			stackPtFrame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
	/* clear the page-table */
	kstackPtAddr = ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,kstackAddr);
	FLUSHADDR(kstackPtAddr);
	memclear((void*)kstackPtAddr,PAGE_SIZE);

	paging_doUnmapFrom(cur,TEMP_MAP_AREA,1,false);

	/* one final flush to ensure everything is correct */
	paging_flushTLB();
	pdir->own = pdirFrame << PAGE_SIZE_SHIFT;
	pdir->other = NULL;
	pdir->lastChange = cpu_rdtsc();
	pdir->otherUpdate = 0;
	if(kstackAddr == KERNEL_STACK_AREA)
		pdir->freeKStack = KERNEL_STACK_AREA + PAGE_SIZE;
	else
		pdir->freeKStack = KERNEL_STACK_AREA;
	spinlock_release(&pagingLock);
	return 0;
}

uintptr_t paging_createKernelStack(pagedir_t *pdir) {
	uintptr_t addr,end,ptables;
	pte_t *pt;
	pde_t *pd;
	spinlock_aquire(&pagingLock);
	addr = pdir->freeKStack;
	end = KERNEL_STACK_AREA + KERNEL_STACK_AREA_SIZE;
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	pd = (pde_t*)PAGEDIR(ptables);
	while(addr < end) {
		pt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,addr);
		/* use this address if either the page-table or the page doesn't exist yet */
		if(!(pd[ADDR_TO_PDINDEX(addr)] & PDE_EXISTS) || !(*pt & PTE_EXISTS)) {
			if(paging_doMapTo(pdir,addr,NULL,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
				addr = 0;
			break;
		}
		addr += PAGE_SIZE;
	}
	if(addr == end)
		addr = 0;
	else
		pdir->freeKStack = addr + PAGE_SIZE;
	spinlock_release(&pagingLock);
	return addr;
}

void paging_removeKernelStack(pagedir_t *pdir,uintptr_t addr) {
	spinlock_aquire(&pagingLock);
	if(addr < pdir->freeKStack)
		pdir->freeKStack = addr;
	paging_doUnmapFrom(pdir,addr,1,true);
	spinlock_release(&pagingLock);
}

void paging_destroyPDir(pagedir_t *pdir) {
	size_t i;
	uintptr_t ptables;
	pde_t *pd;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	pd = (pde_t*)PAGEDIR(ptables);
	/* free page-tables for kernel-stack */
	for(i = ADDR_TO_PDINDEX(KERNEL_STACK_AREA);
		i < ADDR_TO_PDINDEX(KERNEL_STACK_AREA + KERNEL_STACK_AREA_SIZE); i++) {
		if(pd[i] & PDE_EXISTS) {
			pmem_free(PDE_FRAMENO(pd[i]),FRM_KERNEL);
			pd[i] = 0;
		}
	}
	/* free page-dir */
	pmem_free(pdir->own >> PAGE_SIZE_SHIFT,FRM_KERNEL);
	spinlock_release(&pagingLock);
}

bool paging_isPresent(pagedir_t *pdir,uintptr_t virt) {
	uintptr_t ptables;
	pde_t *pd;
	bool res = false;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	pd = (pde_t*)PAGEDIR(ptables);
	if((pd[ADDR_TO_PDINDEX(virt)] & (PDE_PRESENT | PDE_EXISTS)) == (PDE_PRESENT | PDE_EXISTS)) {
		pte_t *pt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		res = (*pt & (PTE_PRESENT | PTE_EXISTS)) == (PTE_PRESENT | PTE_EXISTS);
	}
	spinlock_release(&pagingLock);
	return res;
}

frameno_t paging_getFrameNo(pagedir_t *pdir,uintptr_t virt) {
	uintptr_t ptables;
	frameno_t res = -1;
	pte_t *pt;
	pde_t *pde;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	pde = (pde_t*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(virt);
	pt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	res = PTE_FRAMENO(*pt);
	spinlock_release(&pagingLock);
	assert((*pde & (PDE_PRESENT | PDE_EXISTS)) == (PDE_PRESENT | PDE_EXISTS));
	assert((*pt & (PTE_PRESENT | PTE_EXISTS)) == (PTE_PRESENT | PTE_EXISTS));
	return res;
}

uintptr_t paging_getAccess(frameno_t frame) {
	return paging_mapToTemp(&frame,1);
}

void paging_removeAccess(void) {
	paging_unmapFromTemp(1);
}

frameno_t paging_demandLoad(void *buffer,size_t loadCount,A_UNUSED ulong regFlags) {
	frameno_t frame;
	pagedir_t *pdir = paging_getPageDir();
	spinlock_aquire(&pagingLock);
	frame = thread_getFrame();
	paging_doMapTo(pdir,TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,buffer,loadCount);
	paging_doUnmapFrom(pdir,TEMP_MAP_AREA,1,false);
	spinlock_release(&pagingLock);
	return frame;
}

void paging_copyToFrame(frameno_t frame,const void *src) {
	pagedir_t *pdir = paging_getPageDir();
	spinlock_aquire(&pagingLock);
	paging_doMapTo(pdir,TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,src,PAGE_SIZE);
	paging_doUnmapFrom(pdir,TEMP_MAP_AREA,1,false);
	spinlock_release(&pagingLock);
}

void paging_copyFromFrame(frameno_t frame,void *dst) {
	pagedir_t *pdir = paging_getPageDir();
	spinlock_aquire(&pagingLock);
	paging_doMapTo(pdir,TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy(dst,(void*)TEMP_MAP_AREA,PAGE_SIZE);
	paging_doUnmapFrom(pdir,TEMP_MAP_AREA,1,false);
	spinlock_release(&pagingLock);
}

void paging_copyToUser(void *dst,const void *src,size_t count) {
	/* in this case, we know that we can write to the frame, but it may be mapped as readonly */
	paging_setWriteProtection(false);
	memcpy(dst,src,count);
	paging_setWriteProtection(true);
}

void paging_zeroToUser(void *dst,size_t count) {
	paging_setWriteProtection(false);
	memclear(dst,count);
	paging_setWriteProtection(true);
}

ssize_t paging_clonePages(pagedir_t *src,pagedir_t *dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	pagedir_t *cur = paging_getPageDir();
	ssize_t pts = 0;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	uintptr_t srctables,dsttables;
	pde_t *dstpdir;
	assert(src != dst && (src == cur || dst == cur));
	spinlock_aquire(&pagingLock);
	srctables = paging_getPTables(cur,src);
	dsttables = paging_getPTables(cur,dst);
	dstpdir = (pde_t*)PAGEDIR(dsttables);
	while(count > 0) {
		pte_t *dpt;
		pte_t *spt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(srctables,virtSrc);
		pte_t pte = *spt;
		/* page table not present? */
		if(!(dstpdir[ADDR_TO_PDINDEX(virtDst)] & PDE_PRESENT)) {
			if(paging_crtPageTable(dstpdir,dsttables,virtDst,pte) < 0)
				goto error;
			pts++;
		}
		dpt = (pte_t*)ADDR_TO_MAPPED_CUSTOM(dsttables,virtDst);

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_PRESENT)))
			pte &= ~PTE_WRITABLE;
		*dpt = pte;
		if(dst == cur)
			FLUSHADDR(virtDst);

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_PRESENT)) {
			*spt &= ~PTE_WRITABLE;
			if(src == cur)
				FLUSHADDR(virtSrc);
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	spinlock_release(&pagingLock);
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	paging_unmapFrom(dst,orgVirtDst,orgCount - count,false);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		pte_t *pte = (pte_t*)ADDR_TO_MAPPED_CUSTOM(srctables,orgVirtSrc);
		if(!share && (*pte & PTE_PRESENT))
			paging_doMapTo(src,orgVirtSrc,NULL,1,PG_PRESENT | PG_WRITABLE | PG_KEEPFRM);
		orgVirtSrc += PAGE_SIZE;
		orgCount--;
	}
	spinlock_release(&pagingLock);
	return -ENOMEM;
}

ssize_t paging_map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	return paging_mapTo(paging_getPageDir(),virt,frames,count,flags);
}

ssize_t paging_mapTo(pagedir_t *pdir,uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	ssize_t pts;
	spinlock_aquire(&pagingLock);
	pts = paging_doMapTo(pdir,virt,frames,count,flags);
	spinlock_release(&pagingLock);
	return pts;
}

static int paging_crtPageTable(pde_t *pd,uintptr_t ptables,uintptr_t virt,uint flags) {
	frameno_t frame;
	size_t pdi = ADDR_TO_PDINDEX(virt);
	assert((pd[pdi] & PDE_PRESENT) == 0);
	/* get new frame for page-table */
	frame = pmem_allocate(FRM_KERNEL);
	if(frame == 0)
		return -ENOMEM;
	/* writable because we want to be able to change PTE's in the PTE-area */
	/* is there another reason? :) */
	pd[pdi] = frame << PAGE_SIZE_SHIFT | PDE_PRESENT | PDE_WRITABLE | PDE_EXISTS;
	if(!(flags & PG_SUPERVISOR))
		pd[pdi] |= PDE_NOTSUPER;

	paging_flushPageTable(virt,ptables);
	/* clear frame (ensure that we start at the beginning of the frame) */
	memclear((void*)ADDR_TO_MAPPED_CUSTOM(ptables,
			virt & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),PAGE_SIZE);
	return 0;
}

static ssize_t paging_doMapTo(pagedir_t *pdir,uintptr_t virt,const frameno_t *frames,
		size_t count,uint flags) {
	ssize_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	pagedir_t *cur = paging_getPageDir();
	uintptr_t ptables = paging_getPTables(cur,pdir);
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
			if(paging_crtPageTable(pdirAddr,ptables,virt,flags) < 0)
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
					frame = pmem_allocate(virt < KERNEL_STACK_AREA ? FRM_CRIT : FRM_KERNEL);
					if(frame == 0)
						goto error;
				}
				else
					frame = thread_getFrame();
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
		if(pdir == cur)
			FLUSHADDR(virt);

		/* to next page */
		virt += PAGE_SIZE;
		count--;
	}

	pdir->lastChange = cpu_rdtsc();
	smp_flushTLB(pdir);
	return pts;

error:
	freeFrames = frames == NULL && !(flags & PG_KEEPFRM) && (flags & PG_PRESENT);
	paging_doUnmapFrom(pdir,orgVirt,orgCount - count,freeFrames);
	return -ENOMEM;
}

size_t paging_unmap(uintptr_t virt,size_t count,bool freeFrames) {
	return paging_unmapFrom(paging_getPageDir(),virt,count,freeFrames);
}

size_t paging_unmapFrom(pagedir_t *pdir,uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts;
	spinlock_aquire(&pagingLock);
	pts = paging_doUnmapFrom(pdir,virt,count,freeFrames);
	spinlock_release(&pagingLock);
	return pts;
}

static size_t paging_doUnmapFrom(pagedir_t *pdir,uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts = 0;
	pagedir_t *cur = paging_getPageDir();
	uintptr_t ptables = paging_getPTables(cur,pdir);
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	pte_t *pte = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				pts += paging_remEmptyPt(ptables,lastPti);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		if(*pte & PTE_PRESENT) {
			*pte &= ~PTE_PRESENT;
			if(freeFrames) {
				frameno_t frame = PTE_FRAMENO(*pte);
				if(virt >= KERNEL_AREA) {
					if(virt < KERNEL_STACK_AREA)
						pmem_free(frame,FRM_CRIT);
					else
						pmem_free(frame,FRM_KERNEL);
				}
				else
					pmem_free(frame,FRM_USER);
			}
		}
		*pte &= ~PTE_EXISTS;

		/* invalidate TLB-entry */
		if(pdir == cur)
			FLUSHADDR(virt);

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
		pts += paging_remEmptyPt(ptables,pti);
	pdir->lastChange = cpu_rdtsc();
	smp_flushTLB(pdir);
	return pts;
}

size_t paging_getPTableCount(pagedir_t *pdir) {
	size_t i,count = 0;
	uintptr_t ptables;
	pde_t *pdirAddr;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	pdirAddr = (pde_t*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdirAddr[i] & PDE_PRESENT)
			count++;
	}
	spinlock_release(&pagingLock);
	return count;
}

void paging_sprintfVirtMem(sStringBuffer *buf,pagedir_t *pdir) {
	size_t i,j;
	uintptr_t ptables;
	pde_t *pdirAddr;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(paging_getPageDir(),pdir);
	/* note that we release the lock here because prf_sprintf() might cause the cache-module to
	 * allocate more memory and use paging to map it. therefore, we would cause a deadlock if
	 * we use don't release it here.
	 * I think, in this case we can do it without lock because the only things that could happen
	 * are a delivery of wrong information to the user and a segfault, which would kill the
	 * process. Of course, only the process that is reading the information could cause these
	 * problems. Therefore, its not really a bad thing. */
	spinlock_release(&pagingLock);
	pdirAddr = (pde_t*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdirAddr[i] & PDE_PRESENT) {
			uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * i;
			pte_t *pte = (pte_t*)(ptables + i * PAGE_SIZE);
			prf_sprintf(buf,"PageTable 0x%x (VM: 0x%08x - 0x%08x)\n",i,addr,
					addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
			for(j = 0; j < PT_ENTRY_COUNT; j++) {
				pte_t page = pte[j];
				if(page & PTE_EXISTS) {
					prf_sprintf(buf,"\tPage 0x%03x: ",j);
					prf_sprintf(buf,"frame 0x%05x [%c%c%c] (VM: 0x%08x - 0x%08x)\n",
							PTE_FRAMENO(page),(page & PTE_PRESENT) ? 'p' : '-',
						    (page & PTE_NOTSUPER) ? 'u' : 'k',
						    (page & PTE_WRITABLE) ? 'w' : 'r',
							addr,addr + PAGE_SIZE - 1);
				}
				addr += PAGE_SIZE;
			}
		}
	}
}

static pagedir_t *paging_getPageDir(void) {
	return proc_getPageDir();
}

static uintptr_t paging_getPTables(pagedir_t *cur,pagedir_t *other) {
	pde_t *pde;
	if(other == cur)
		return MAPPED_PTS_START;
	if(other == cur->other) {
		/* do we have to refresh the mapping? */
		if(other->lastChange <= cur->otherUpdate)
			return TMPMAP_PTS_START;
	}
	/* map page-tables to other area */
	pde = ((pde_t*)PAGE_DIR_AREA) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	*pde = other->own | PDE_PRESENT | PDE_EXISTS | PDE_WRITABLE;
	/* we want to access the whole page-table */
	paging_flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	cur->other = other;
	cur->otherUpdate = cpu_rdtsc();
	return TMPMAP_PTS_START;
}

static size_t paging_remEmptyPt(uintptr_t ptables,size_t pti) {
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
	pmem_free(PDE_FRAMENO(*pde),FRM_KERNEL);
	*pde = 0;
	if(ptables == MAPPED_PTS_START)
		paging_flushPageTable(virt,ptables);
	else
		FLUSHADDR((uintptr_t)pte);
	return 1;
}

static void paging_flushPageTable(A_UNUSED uintptr_t virt,A_UNUSED uintptr_t ptables) {
	/* it seems to be much faster to flush the complete TLB instead of flushing just the
	 * affected range (and hoping that less TLB-misses occur afterwards) */
	paging_flushTLB();
}

size_t paging_dbg_getPageCount(void) {
	size_t i,x,count = 0;
	pde_t *pdir = (pde_t*)PAGE_DIR_AREA;
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdir[i] & PDE_PRESENT) {
			pte_t *pt = (pte_t*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pt[x] & PTE_PRESENT)
					count++;
			}
		}
	}
	return count;
}

void paging_printPageOf(pagedir_t *pdir,uintptr_t virt) {
	/* don't use locking here; its only used in the debugging-console anyway. its better without
	 * locking to be able to view these things even if somebody currently holds the lock (e.g.
	 * because a panic occurred during that operation) */
	pagedir_t *cur = proc_getPageDir();
	pagedir_t *old = cur->other;
	uintptr_t ptables;
	pde_t *pdirAddr;
	ptables = paging_getPTables(cur,pdir);
	pdirAddr = (pde_t*)PAGEDIR(ptables);
	if(pdirAddr[ADDR_TO_PDINDEX(virt)] & PDE_PRESENT) {
		pte_t *page = (pte_t*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		vid_printf("Page @ %p: ",virt);
		paging_printPage(*page);
		vid_printf("\n");
	}
	/* restore the old mapping, if necessary */
	if(cur->other != old)
		paging_getPTables(cur,old);
}

void paging_printCur(uint parts) {
	paging_printPDir(paging_getPageDir(),parts);
}

void paging_printPDir(pagedir_t *pdir,uint parts) {
	size_t i;
	pagedir_t *cur = proc_getPageDir();
	pagedir_t *old = cur->other;
	uintptr_t ptables;
	pde_t *pdirAddr;
	ptables = paging_getPTables(cur,pdir);
	pdirAddr = (pde_t*)PAGEDIR(ptables);
	vid_printf("page-dir @ %p:\n",pdirAddr);
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
			paging_printPageTable(ptables,i,pdirAddr[i]);
		}
	}
	vid_printf("\n");
	if(cur->other != old)
		paging_getPTables(cur,old);
}

static void paging_printPageTable(uintptr_t ptables,size_t no,pde_t pde) {
	size_t i;
	uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	pte_t *pte = (pte_t*)(ptables + no * PAGE_SIZE);
	vid_printf("\tpt 0x%x [frame 0x%x, %c%c] @ %p: (VM: %p - %p)\n",no,
			PDE_FRAMENO(pde),(pde & PDE_NOTSUPER) ? 'u' : 'k',
			(pde & PDE_WRITABLE) ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & PTE_EXISTS) {
				vid_printf("\t\t0x%zx: ",i);
				paging_printPage(pte[i]);
				vid_printf(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_printPage(pte_t page) {
	if(page & PTE_EXISTS) {
		vid_printf("r=0x%08x fr=0x%x [%c%c%c%c]",page,PTE_FRAMENO(page),
				(page & PTE_PRESENT) ? 'p' : '-',(page & PTE_NOTSUPER) ? 'u' : 'k',
				(page & PTE_WRITABLE) ? 'w' : 'r',(page & PTE_GLOBAL) ? 'g' : '-');
	}
	else {
		vid_printf("-");
	}
}
