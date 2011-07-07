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
#include <sys/mem/swap.h>
#include <sys/mem/swapmap.h>
#include <sys/mem/sharedmem.h>
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/intrpt.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/printf.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

/* to shift a flag down to the first bit */
#define PG_PRESENT_SHIFT	0
#define PG_WRITABLE_SHIFT	1
#define PG_SUPERVISOR_SHIFT	3
#define PG_GLOBAL_SHIFT		5

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + \
								(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + \
		(((uintptr_t)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))

#define PAGEDIR(ptables)	((uintptr_t)(ptables) + PAGE_SIZE * (PT_ENTRY_COUNT - 1))

/**
 * Flushes the TLB-entry for the given virtual address.
 * NOTE: supported for >= Intel486
 */
#define	FLUSHADDR(addr)		__asm__ __volatile__ ("invlpg (%0)" : : "r" (addr));

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000)
 * Note that this does not work anymore as soon as the GDT is "corrected" and paging enabled! */
#define VIRT2PHYS(addr)		((uintptr_t)(addr) + 0x40000000)

/* represents a page-directory-entry */
typedef struct {
	/* 1 if the page is present in memory */
	uint32_t present	: 1,
	/* whether the page is writable */
	writable			: 1,
	/* if enabled the page may be used by privilege level 0, 1 and 2 only. */
	notSuperVisor		: 1,
	/* >= 80486 only. if enabled everything will be written immediatly into memory */
	pageWriteThrough	: 1,
	/* >= 80486 only. if enabled the CPU will not put anything from the page in the cache */
	pageCacheDisable	: 1,
	/* enabled if the page-table has been accessed (has to be cleared by the OS!) */
	accessed			: 1,
	/* 1 ignored bit */
						: 1,
	/* whether the pages are 4 KiB (=0) or 4 MiB (=1) large */
	pageSize			: 1,
	/* 1 ignored bit */
						: 1,
	/* can be used by the OS */
	/* Indicates that this pagetable exists (but must not necessarily be present) */
	exists				: 1,
	/* unused */
						: 2,
	/* the physical address of the page-table */
	ptFrameNo			: 20;
} sPDEntry;

/* represents a page-table-entry */
typedef struct {
	/* 1 if the page is present in memory */
	uint32_t present	: 1,
	/* whether the page is writable */
	writable			: 1,
	/* if enabled the page may be used by privilege level 0, 1 and 2 only. */
	notSuperVisor		: 1,
	/* >= 80486 only. if enabled everything will be written immediatly into memory */
	pageWriteThrough	: 1,
	/* >= 80486 only. if enabled the CPU will not put anything from the page in the cache */
	pageCacheDisable	: 1,
	/* enabled if the page has been accessed (has to be cleared by the OS!) */
	accessed			: 1,
	/* enabled if the page can not be removed currently (has to be cleared by the OS!) */
	dirty				: 1,
	/* 1 ignored bit */
						: 1,
	/* The Global, or 'G' above, flag, if set, prevents the TLB from updating the address in
	 * it's cache if CR3 is reset. Note, that the page global enable bit in CR4 must be set
	 * to enable this feature. (>= pentium pro) */
	global				: 1,
	/* 3 Bits for the OS */
	/* Indicates that this page exists (but must not necessarily be present) */
	exists				: 1,
	/* unused */
						: 2,
	/* the physical address of the page */
	frameNumber			: 20;
} sPTEntry;

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

static uintptr_t paging_getPTables(tPageDir pdir);
static size_t paging_remEmptyPt(uintptr_t ptables,size_t pti);

static void paging_printPageTable(uintptr_t ptables,size_t no,sPDEntry *pde) ;
static void paging_printPage(sPTEntry *page);

/* the page-directory for process 0 */
static sPDEntry proc0PD[PAGE_SIZE / sizeof(sPDEntry)] A_ALIGNED(PAGE_SIZE);
/* the page-tables for process 0 (two because our mm-stack may get large if we have a lot physmem) */
static sPTEntry proc0PT1[PAGE_SIZE / sizeof(sPTEntry)] A_ALIGNED(PAGE_SIZE);
static sPTEntry proc0PT2[PAGE_SIZE / sizeof(sPTEntry)] A_ALIGNED(PAGE_SIZE);

static tPageDir curPDir = 0;
static tPageDir otherPDir = 0;

void paging_init(void) {
	sPDEntry *pd,*pde;
	sPTEntry *pt;
	size_t j,i;
	uintptr_t vaddr,addr,end;
	sPTEntry *pts[] = {proc0PT1,proc0PT2};

	/* set page-dir of first process */
	curPDir = (tPageDir)proc0PD & ~KERNEL_START;

	/* note that we assume here that the kernel including mm-stack is not larger than 2
	 * complete page-tables (8MiB)! */

	/* map 2 page-tables at 0xC0000000 */
	vaddr = KERNEL_START;
	addr = KERNEL_AREA_P_ADDR;
	pd = (sPDEntry*)VIRT2PHYS(proc0PD);
	for(j = 0; j < 2; j++) {
		pt = (sPTEntry*)VIRT2PHYS(pts[j]);

		/* map one page-table */
		end = addr + (PT_ENTRY_COUNT) * PAGE_SIZE;
		for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
			/* build page-table entry */
			pts[j][i].frameNumber = (uintptr_t)addr >> PAGE_SIZE_SHIFT;
			pts[j][i].global = true;
			pts[j][i].present = true;
			pts[j][i].writable = true;
			pts[j][i].exists = true;
		}

		/* insert page-table in the page-directory */
		pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(vaddr));
		pde->ptFrameNo = (uintptr_t)pt >> PAGE_SIZE_SHIFT;
		pde->present = true;
		pde->writable = true;
		pde->exists = true;

		/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
		pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(vaddr - KERNEL_START));
		pde->ptFrameNo = (uintptr_t)pt >> PAGE_SIZE_SHIFT;
		pde->present = true;
		pde->writable = true;
		pde->exists = true;
		vaddr += PT_ENTRY_COUNT * PAGE_SIZE;
	}

	/* put the page-directory in the last page-dir-slot */
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptFrameNo = (uintptr_t)pd >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;
	pde->exists = true;

	/* now set page-dir and enable paging */
	paging_exchangePDir((tPageDir)pd);
	paging_enable();
	/* enable global pages (TODO just possible for >= pentium pro (family 6)) */
	cpu_setCR4(cpu_getCR4() | (1 << 7));
}

tPageDir paging_getCur(void) {
	return curPDir;
}

void paging_setCur(tPageDir pdir) {
	curPDir = pdir;
	/* we don't know the other one (or at least, can't find it out very easily here) */
	otherPDir = 0;
}

void paging_mapKernelSpace(void) {
	uintptr_t addr,end;
	sPDEntry *pde;
	/* insert all page-tables for 0xC0800000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_START + (PAGE_SIZE * PT_ENTRY_COUNT * 2);
	end = KERNEL_STACK;
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		frameno_t frame = pmem_allocate();
		pde->ptFrameNo = frame;
		pde->present = true;
		pde->writable = true;
		pde->exists = true;
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
	proc0PD[0].present = false;
	proc0PD[0].exists = false;
	proc0PD[1].present = false;
	proc0PD[1].exists = false;
	paging_flushTLB();
}

/* TODO perhaps we should move isRange*() to vmm? */

bool paging_isRangeUserReadable(uintptr_t virt,size_t count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > KERNEL_START || virt + count < virt)
		return false;

	return paging_isRangeReadable(virt,count);
}

bool paging_isRangeReadable(uintptr_t virt,size_t count) {
	sPTEntry *pt;
	sPDEntry *pd;
	uintptr_t end;
	/* calc start and end pt */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	while(virt < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virt);
		if(!pd->present || !pt->exists)
			return false;
		if(!pt->present) {
			/* we have tp handle the page-fault here */
			if(!vmm_pagefault(virt))
				return false;
		}
		virt += PAGE_SIZE;
		pt++;
	}
	return true;
}

bool paging_isRangeUserWritable(uintptr_t virt,size_t count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > KERNEL_START || virt + count < virt)
		return false;

	return paging_isRangeWritable(virt,count);
}

bool paging_isRangeWritable(uintptr_t virt,size_t count) {
	sPTEntry *pt;
	sPDEntry *pd;
	uintptr_t end;
	/* calc start and end pt */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	while(virt < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virt);
		if(!pd->present || !pt->exists)
			return false;
		if(!pt->present || !pt->writable) {
			if(!vmm_pagefault(virt))
				return false;
		}
		virt += PAGE_SIZE;
		pt++;
	}
	return true;
}

uintptr_t paging_mapToTemp(const frameno_t *frames,size_t count) {
	assert(count <= TEMP_MAP_AREA_SIZE / PAGE_SIZE);
	paging_map(TEMP_MAP_AREA,frames,count,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	return TEMP_MAP_AREA;
}

void paging_unmapFromTemp(size_t count) {
	paging_unmap(TEMP_MAP_AREA,count,false);
}

ssize_t paging_cloneKernelspace(frameno_t *stackFrame,tPageDir *pdir) {
	uintptr_t kstackAddr;
	frameno_t pdirFrame;
	ssize_t frmCount = 0;
	sPDEntry *pd,*npd,*tpd;
	sPTEntry *pt;
	sProc *p;
	sThread *curThread = thread_getRunning();

	/* frames needed:
	 * 	- page directory
	 * 	- kernel-stack page-table
	 * 	- kernel stack
	 */
	p = curThread->proc;
	if(pmem_getFreeFrames(MM_DEF) < 3)
		return ERR_NOT_ENOUGH_MEM;

	/* we need a new page-directory */
	pdirFrame = pmem_allocate();
	frmCount++;

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	pd = (sPDEntry*)PAGE_DIR_AREA;
	npd = (sPDEntry*)paging_mapToTemp(&pdirFrame,1);

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_START) * sizeof(sPDEntry));
	/* copy kernel-space page-tables */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_START),
			pd + ADDR_TO_PDINDEX(KERNEL_START),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_START)) * sizeof(sPDEntry));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK);
	tpd->ptFrameNo = pmem_allocate();
	tpd->present = true;
	tpd->writable = true;
	tpd->exists = true;
	frmCount++;
	/* clear the page-table */
	kstackAddr = ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,
			KERNEL_STACK & ~(PT_ENTRY_COUNT * PAGE_SIZE - 1));
	FLUSHADDR(kstackAddr);
	otherPDir = 0;
	memclear((void*)kstackAddr,PAGE_SIZE);

	/* create stack-page */
	pt = (sPTEntry*)(TMPMAP_PTS_START + (KERNEL_STACK / PT_ENTRY_COUNT));
	pt->frameNumber = pmem_allocate();
	*stackFrame = pt->frameNumber;
	pt->present = true;
	pt->writable = true;
	pt->exists = true;
	frmCount++;

	paging_unmapFromTemp(1);

	/* one final flush to ensure everything is correct */
	paging_flushTLB();
	*pdir = pdirFrame << PAGE_SIZE_SHIFT;
	return frmCount;
}

sAllocStats paging_destroyPDir(tPageDir pdir) {
	sAllocStats stats;
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pde;
	assert(pdir != curPDir);
	/* remove kernel-stack (don't free the frame; its done in thread_kill()) */
	stats = paging_unmapFrom(pdir,KERNEL_STACK,1,false);
	/* free page-table for kernel-stack */
	pde = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(KERNEL_STACK);
	pde->present = false;
	pde->exists = false;
	pmem_free(pde->ptFrameNo);
	stats.ptables++;
	/* free page-dir */
	pmem_free(pdir >> PAGE_SIZE_SHIFT);
	stats.ptables++;
	/* ensure that we don't use it again */
	otherPDir = 0;
	return stats;
}

bool paging_isPresent(tPageDir pdir,uintptr_t virt) {
	uintptr_t ptables = paging_getPTables(pdir);
	sPTEntry *pt;
	sPDEntry *pde = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(virt);
	if(!pde->present || !pde->exists)
		return false;
	pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	return pt->present && pt->exists;
}

frameno_t paging_getFrameNo(tPageDir pdir,uintptr_t virt) {
	uintptr_t ptables = paging_getPTables(pdir);
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	assert(pt->present && pt->exists);
	return pt->frameNumber;
}

frameno_t paging_demandLoad(void *buffer,size_t loadCount,ulong regFlags) {
	UNUSED(regFlags);
	frameno_t frame = pmem_allocate();
	uintptr_t temp = paging_mapToTemp(&frame,1);
	memcpy((void*)temp,buffer,loadCount);
	paging_unmapFromTemp(1);
	return frame;
}

void paging_copyToUser(void *dst,const void *src,size_t count) {
	memcpy(dst,src,count);
}

void paging_zeroToUser(void *dst,size_t count) {
	memclear(dst,count);
}

sAllocStats paging_clonePages(tPageDir src,tPageDir dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	sAllocStats stats = {0,0};
	uintptr_t srctables = paging_getPTables(src);
	assert(src != dst && (src == curPDir || dst == curPDir));
	while(count-- > 0) {
		sAllocStats mstats;
		sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(srctables,virtSrc);
		frameno_t *frames = NULL;
		uint flags = 0;
		if(pte->present)
			flags |= PG_PRESENT;
		if(pte->global)
			flags |= PG_GLOBAL;
		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if(pte->writable && (share || !pte->present))
			flags |= PG_WRITABLE;
		if(!pte->notSuperVisor)
			flags |= PG_SUPERVISOR;
		if(share || pte->present) {
			flags |= PG_ADDR_TO_FRAME;
			frames = (frameno_t*)pte;
		}
		mstats = paging_mapTo(dst,virtDst,frames,1,flags);
		if(flags & PG_PRESENT)
			stats.frames++;
		stats.ptables += mstats.ptables;
		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && pte->present)
			paging_mapTo(src,virtSrc,NULL,1,flags | PG_KEEPFRM);
		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
	}
	return stats;
}

sAllocStats paging_map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	return paging_mapTo(curPDir,virt,frames,count,flags);
}

sAllocStats paging_mapTo(tPageDir pdir,uintptr_t virt,const frameno_t *frames,size_t count,
		uint flags) {
	sAllocStats stats = {0,0};
	uintptr_t ptables = paging_getPTables(pdir);
	uintptr_t pdirAddr = PAGEDIR(ptables);
	sPDEntry *pde;
	sPTEntry *pte;

	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		pde = (sPDEntry*)pdirAddr + ADDR_TO_PDINDEX(virt);
		/* page table not present? */
		if(!pde->present) {
			/* get new frame for page-table */
			pde->ptFrameNo = pmem_allocate();
			pde->present = true;
			pde->exists = true;
			/* writable because we want to be able to change PTE's in the PTE-area */
			/* is there another reason? :) */
			pde->writable = true;
			pde->notSuperVisor = !((flags & PG_SUPERVISOR) >> PG_SUPERVISOR_SHIFT);
			stats.ptables++;

			paging_flushPageTable(virt,ptables);
			/* clear frame (ensure that we start at the beginning of the frame) */
			memclear((void*)ADDR_TO_MAPPED_CUSTOM(ptables,
					virt & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),PAGE_SIZE);
		}

		/* setup page */
		pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		/* ignore already present entries */
		pte->present = (flags & PG_PRESENT) >> PG_PRESENT_SHIFT;
		pte->exists = true;
		pte->global = (flags & PG_GLOBAL) >> PG_GLOBAL_SHIFT;
		pte->notSuperVisor = !((flags & PG_SUPERVISOR) >> PG_SUPERVISOR_SHIFT);
		pte->writable = (flags & PG_WRITABLE) >> PG_WRITABLE_SHIFT;

		/* set frame-number */
		if(!(flags & PG_KEEPFRM) && (flags & PG_PRESENT)) {
			if(frames == NULL) {
				pte->frameNumber = pmem_allocate();
				stats.frames++;
			}
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pte->frameNumber = *frames++ >> PAGE_SIZE_SHIFT;
				else
					pte->frameNumber = *frames++;
			}
		}

		/* invalidate TLB-entry */
		if(pdir == curPDir)
			FLUSHADDR(virt);

		/* to next page */
		virt += PAGE_SIZE;
	}

	return stats;
}

sAllocStats paging_unmap(uintptr_t virt,size_t count,bool freeFrames) {
	return paging_unmapFrom(curPDir,virt,count,freeFrames);
}

sAllocStats paging_unmapFrom(tPageDir pdir,uintptr_t virt,size_t count,bool freeFrames) {
	sAllocStats stats = {0,0};
	uintptr_t ptables = paging_getPTables(pdir);
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_START)
				stats.ptables += paging_remEmptyPt(ptables,lastPti);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		if(pte->present) {
			pte->present = false;
			if(freeFrames) {
				pmem_free(pte->frameNumber);
				stats.frames++;
			}
		}
		pte->exists = false;

		/* invalidate TLB-entry */
		if(pdir == curPDir)
			FLUSHADDR(virt);

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_START)
		stats.ptables += paging_remEmptyPt(ptables,pti);
	return stats;
}

size_t paging_getPTableCount(tPageDir pdir) {
	size_t i,count = 0;
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_START); i++) {
		if(pdirAddr[i].present)
			count++;
	}
	return count;
}

void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir pdir) {
	size_t i,j;
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_START); i++) {
		if(pdirAddr[i].present) {
			uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * i;
			sPTEntry *pte = (sPTEntry*)(ptables + i * PAGE_SIZE);
			prf_sprintf(buf,"PageTable 0x%x (VM: 0x%08x - 0x%08x)\n",i,addr,
					addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
			for(j = 0; j < PT_ENTRY_COUNT; j++) {
				if(pte[j].exists) {
					sPTEntry *page = pte + j;
					prf_sprintf(buf,"\tPage 0x%03x: ",j);
					prf_sprintf(buf,"frame 0x%05x [%c%c%c] (VM: 0x%08x - 0x%08x)\n",
							page->frameNumber,page->present ? 'p' : '-',
							page->notSuperVisor ? 'u' : 'k',
							page->writable ? 'w' : 'r',
							addr,addr + PAGE_SIZE - 1);
				}
				addr += PAGE_SIZE;
			}
		}
	}
}

static uintptr_t paging_getPTables(tPageDir pdir) {
	sPDEntry *pde;
	if(pdir == curPDir)
		return MAPPED_PTS_START;
	if(pdir == otherPDir)
		return TMPMAP_PTS_START;
	/* map page-tables to other area*/
	pde = ((sPDEntry*)PAGE_DIR_AREA) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	pde->present = true;
	pde->exists = true;
	pde->notSuperVisor = false;
	pde->ptFrameNo = pdir >> PAGE_SIZE_SHIFT;
	pde->writable = true;
	/* we want to access the whole page-table */
	paging_flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	otherPDir = pdir;
	return TMPMAP_PTS_START;
}

static size_t paging_remEmptyPt(uintptr_t ptables,size_t pti) {
	size_t i;
	sPDEntry *pde;
	uintptr_t virt = pti * PAGE_SIZE * PT_ENTRY_COUNT;
	sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pte[i].exists)
			return 0;
	}
	/* empty => can be removed */
	pde = (sPDEntry*)PAGEDIR(ptables) + pti;
	pde->present = false;
	pde->exists = false;
	pmem_free(pde->ptFrameNo);
	if(ptables == MAPPED_PTS_START)
		paging_flushPageTable(virt,ptables);
	else
		FLUSHADDR((uintptr_t)pte);
	return 1;
}

static void paging_flushPageTable(uintptr_t virt,uintptr_t ptables) {
	uintptr_t end;
	/* to beginning of page-table */
	virt &= ~(PT_ENTRY_COUNT * PAGE_SIZE - 1);
	end = virt + PT_ENTRY_COUNT * PAGE_SIZE;
	/* flush page-table in mapped area */
	FLUSHADDR(ADDR_TO_MAPPED_CUSTOM(ptables,virt));
	/* flush pages in the page-table */
	while(virt < end) {
		FLUSHADDR(virt);
		virt += PAGE_SIZE;
	}
}

#if 0
void paging_getFrameNos(frameno_t *nos,uintptr_t addr,size_t size) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(addr);
	size_t pcount = BYTES_2_PAGES((addr & (PAGE_SIZE - 1)) + size);
	while(pcount-- > 0) {
		*nos++ = pt->frameNumber;
		pt++;
	}
}
#endif

size_t paging_dbg_getPageCount(void) {
	size_t i,x,count = 0;
	sPTEntry *pagetable;
	sPDEntry *pdir = (sPDEntry*)PAGE_DIR_AREA;
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_START); i++) {
		if(pdir[i].present) {
			pagetable = (sPTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pagetable[x].present)
					count++;
			}
		}
	}
	return count;
}

void paging_printPageOf(tPageDir pdir,uintptr_t virt) {
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	if(pdirAddr[ADDR_TO_PDINDEX(virt)].present) {
		sPTEntry *page = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		vid_printf("Page @ %p: ",virt);
		paging_printPage(page);
		vid_printf("\n");
	}
}

void paging_printCur(uint parts) {
	paging_printPDir(curPDir,parts);
}

void paging_printPDir(tPageDir pdir,uint parts) {
	size_t i;
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	vid_printf("page-dir @ %p:\n",pdirAddr);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(!pdirAddr[i].present)
			continue;
		if(parts == PD_PART_ALL ||
			(i < ADDR_TO_PDINDEX(KERNEL_START) && (parts & PD_PART_USER)) ||
			(i >= ADDR_TO_PDINDEX(KERNEL_START) &&
					i < ADDR_TO_PDINDEX(KERNEL_HEAP_START) &&
					(parts & PD_PART_KERNEL)) ||
			(i >= ADDR_TO_PDINDEX(TEMP_MAP_AREA) &&
					i < ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE) &&
					(parts & PD_PART_TEMPMAP)) ||
			(i >= ADDR_TO_PDINDEX(KERNEL_HEAP_START) &&
					i < ADDR_TO_PDINDEX(KERNEL_HEAP_START + KERNEL_HEAP_SIZE) &&
					(parts & PD_PART_KHEAP)) ||
			(i >= ADDR_TO_PDINDEX(MAPPED_PTS_START) && (parts & PD_PART_PTBLS))) {
			paging_printPageTable(ptables,i,pdirAddr + i);
		}
	}
	vid_printf("\n");
}

static void paging_printPageTable(uintptr_t ptables,size_t no,sPDEntry *pde) {
	size_t i;
	uintptr_t addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	sPTEntry *pte = (sPTEntry*)(ptables + no * PAGE_SIZE);
	vid_printf("\tpt 0x%x [frame 0x%x, %c%c] @ %p: (VM: %p - %p)\n",no,
			pde->ptFrameNo,pde->notSuperVisor ? 'u' : 'k',pde->writable ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].exists) {
				vid_printf("\t\t0x%zx: ",i);
				paging_printPage(pte + i);
				vid_printf(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_printPage(sPTEntry *page) {
	if(page->exists) {
		vid_printf("r=0x%08x fr=0x%x [%c%c%c%c]",*(uint32_t*)page,
				page->frameNumber,page->present ? 'p' : '-',
				page->notSuperVisor ? 'u' : 'k',page->writable ? 'w' : 'r',
				page->global ? 'g' : '-');
	}
	else {
		vid_printf("-");
	}
}
