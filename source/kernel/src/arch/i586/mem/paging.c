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

static void paging_setWriteProtection(bool enabled);
static ssize_t paging_doMapTo(tPageDir *pdir,uintptr_t virt,const frameno_t *frames,
		size_t count,uint flags);
static size_t paging_doUnmapFrom(tPageDir *pdir,uintptr_t virt,size_t count,bool freeFrames);
static tPageDir *paging_getPageDir(void);
static uintptr_t paging_getPTables(tPageDir *pdir);
static size_t paging_remEmptyPt(uintptr_t ptables,size_t pti);

static void paging_printPageTable(uintptr_t ptables,size_t no,sPDEntry *pde) ;
static void paging_printPage(sPTEntry *page);

/* the page-directory for process 0 */
static sPDEntry proc0PD[PAGE_SIZE / sizeof(sPDEntry)] A_ALIGNED(PAGE_SIZE);
/* the page-tables for process 0 (two because our mm-stack will get large if we have a lot physmem) */
static sPTEntry proc0PT1[PAGE_SIZE / sizeof(sPTEntry)] A_ALIGNED(PAGE_SIZE);
static sPTEntry proc0PT2[PAGE_SIZE / sizeof(sPTEntry)] A_ALIGNED(PAGE_SIZE);
static klock_t tmpMapLock;
static klock_t pagingLock;

void paging_init(void) {
	sPDEntry *pd,*pde;
	sPTEntry *pt;
	size_t j,i;
	uintptr_t vaddr,addr,end;
	sPTEntry *pts[] = {proc0PT1,proc0PT2};

	/* note that we assume here that the kernel including mm-stack is not larger than 2
	 * complete page-tables (8MiB)! */

	/* map 2 page-tables at 0xC0000000 */
	vaddr = KERNEL_AREA;
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
		pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(vaddr - KERNEL_AREA));
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

void paging_setFirst(tPageDir *pdir) {
	pdir->own = (uintptr_t)proc0PD & ~KERNEL_AREA;
	pdir->other = NULL;
	pdir->lastChange = cpu_rdtsc();
	pdir->otherUpdate = 0;
}

void paging_mapKernelSpace(void) {
	uintptr_t addr,end;
	sPDEntry *pde;
	/* insert all page-tables for 0xC0800000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_AREA + (PAGE_SIZE * PT_ENTRY_COUNT * 2);
	end = KERNEL_STACK_AREA;
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		frameno_t frame = pmem_allocate(true);
		if(frame == 0)
			util_panic("Not enough kernel-memory");
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

int paging_cloneKernelspace(tPageDir *pdir) {
	uintptr_t kstackAddr;
	frameno_t pdirFrame,stackPtFrame;
	sPDEntry *pd,*npd,*tpd;
	tPageDir *cur = paging_getPageDir();
	spinlock_aquire(&pagingLock);

	/* allocate frames */
	pdirFrame = pmem_allocate(true);
	if(pdirFrame == 0)
		return -ENOMEM;
	stackPtFrame = pmem_allocate(true);
	if(stackPtFrame == 0) {
		pmem_free(pdirFrame,true);
		return -ENOMEM;
	}

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	pd = (sPDEntry*)PAGE_DIR_AREA;
	paging_doMapTo(cur,TEMP_MAP_AREA,&pdirFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	npd = (sPDEntry*)TEMP_MAP_AREA;

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(sPDEntry));
	/* copy kernel-space page-tables */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA)) * sizeof(sPDEntry));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK_AREA);
	tpd->ptFrameNo = stackPtFrame;
	tpd->present = true;
	tpd->writable = true;
	tpd->exists = true;
	/* clear the page-table */
	kstackAddr = ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,KERNEL_STACK_AREA);
	FLUSHADDR(kstackAddr);
	memclear((void*)kstackAddr,PAGE_SIZE);

	paging_doUnmapFrom(cur,TEMP_MAP_AREA,1,false);

	/* one final flush to ensure everything is correct */
	paging_flushTLB();
	pdir->own = pdirFrame << PAGE_SIZE_SHIFT;
	pdir->other = NULL;
	pdir->lastChange = cpu_rdtsc();
	pdir->otherUpdate = 0;
	spinlock_release(&pagingLock);
	return 0;
}

uintptr_t paging_createKernelStack(tPageDir *pdir) {
	uintptr_t addr,ptables;
	size_t i;
	sPTEntry *pt;
	sPDEntry *pd;
	spinlock_aquire(&pagingLock);
	addr = 0;
	ptables = paging_getPTables(pdir);
	pd = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(KERNEL_STACK_AREA);
	/* the page-table isn't present for the initial process */
	if(pd->exists) {
		pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,KERNEL_STACK_AREA);
		for(i = PT_ENTRY_COUNT; i > 0; i--) {
			if(!pt[i - 1].exists) {
				addr = KERNEL_STACK_AREA + (i - 1) * PAGE_SIZE;
				break;
			}
		}
	}
	else
		addr = KERNEL_STACK_AREA + (PT_ENTRY_COUNT - 1) * PAGE_SIZE;
	if(addr != 0) {
		if(paging_doMapTo(pdir,addr,NULL,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR) < 0)
			addr = 0;
	}
	spinlock_release(&pagingLock);
	return addr;
}

void paging_destroyPDir(tPageDir *pdir) {
	uintptr_t ptables;
	sPDEntry *pde;
	assert(pdir != paging_getPageDir());
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(pdir);
	/* free page-table for kernel-stack */
	pde = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(KERNEL_STACK_AREA);
	pde->present = false;
	pde->exists = false;
	pmem_free(pde->ptFrameNo,true);
	/* free page-dir */
	pmem_free(pdir->own >> PAGE_SIZE_SHIFT,true);
	spinlock_release(&pagingLock);
}

bool paging_isPresent(tPageDir *pdir,uintptr_t virt) {
	uintptr_t ptables;
	sPTEntry *pt;
	sPDEntry *pde;
	bool res = false;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(pdir);
	pde = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(virt);
	if(pde->present && pde->exists) {
		pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		res = pt->present && pt->exists;
	}
	spinlock_release(&pagingLock);
	return res;
}

frameno_t paging_getFrameNo(tPageDir *pdir,uintptr_t virt) {
	uintptr_t ptables;
	frameno_t res = -1;
	sPTEntry *pt;
	sPDEntry *pde;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(pdir);
	pde = (sPDEntry*)PAGEDIR(ptables) + ADDR_TO_PDINDEX(virt);
	pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	res = pt->frameNumber;
	spinlock_release(&pagingLock);
	assert(pde->present && pde->exists);
	assert(pt->present && pt->exists);
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
	tPageDir *pdir = paging_getPageDir();
	sThread *t = thread_getRunning();
	spinlock_aquire(&pagingLock);
	frame = thread_getFrame(t);
	/* TODO we should disable paging and copy directly in physical space */
	paging_doMapTo(pdir,TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,buffer,loadCount);
	paging_doUnmapFrom(pdir,TEMP_MAP_AREA,1,false);
	spinlock_release(&pagingLock);
	return frame;
}

void paging_copyToFrame(frameno_t frame,const void *src) {
	tPageDir *pdir = paging_getPageDir();
	spinlock_aquire(&pagingLock);
	paging_doMapTo(pdir,TEMP_MAP_AREA,&frame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	memcpy((void*)TEMP_MAP_AREA,src,PAGE_SIZE);
	paging_doUnmapFrom(pdir,TEMP_MAP_AREA,1,false);
	spinlock_release(&pagingLock);
}

void paging_copyFromFrame(frameno_t frame,void *dst) {
	tPageDir *pdir = paging_getPageDir();
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

ssize_t paging_clonePages(tPageDir *src,tPageDir *dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	ssize_t pts = 0;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	uintptr_t srctables;
	tPageDir *cur = paging_getPageDir();
	assert(src != dst && (src == cur || dst == cur));
	spinlock_aquire(&pagingLock);
	srctables = paging_getPTables(src);
	while(count > 0) {
		ssize_t mres;
		sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(srctables,virtSrc);
		frameno_t *frames = NULL;
		uint flags = 0;
		assert(!pte->global && pte->notSuperVisor);
		if(pte->present)
			flags |= PG_PRESENT;
		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if(pte->writable && (share || !pte->present))
			flags |= PG_WRITABLE;
		if(share || pte->present) {
			flags |= PG_ADDR_TO_FRAME;
			frames = (frameno_t*)pte;
		}
		mres = paging_doMapTo(dst,virtDst,frames,1,flags);
		if(mres < 0)
			goto error;
		pts += mres;
		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		/* this mapping can't fail because we will never allocate page-tables or frames */
		if(!share && pte->present)
			paging_doMapTo(src,virtSrc,NULL,1,flags | PG_KEEPFRM);
		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		count--;
	}
	spinlock_release(&pagingLock);
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	paging_doUnmapFrom(dst,orgVirtDst,orgCount - count,false);
	/* make the cow-pages writable again */
	while(orgCount > count) {
		sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(srctables,orgVirtSrc);
		if(!share && pte->present)
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

ssize_t paging_mapTo(tPageDir *pdir,uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	ssize_t pts;
	spinlock_aquire(&pagingLock);
	pts = paging_doMapTo(pdir,virt,frames,count,flags);
	spinlock_release(&pagingLock);
	return pts;
}

static ssize_t paging_doMapTo(tPageDir *pdir,uintptr_t virt,const frameno_t *frames,
		size_t count,uint flags) {
	ssize_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	tPageDir *cur = paging_getPageDir();
	uintptr_t ptables = paging_getPTables(pdir);
	uintptr_t pdirAddr = PAGEDIR(ptables);
	sThread *t = thread_getRunning();
	sPDEntry *pde;
	sPTEntry *pte;
	bool freeFrames;

	virt &= ~(PAGE_SIZE - 1);
	while(count > 0) {
		pde = (sPDEntry*)pdirAddr + ADDR_TO_PDINDEX(virt);
		/* page table not present? */
		if(!pde->present) {
			/* get new frame for page-table */
			pde->ptFrameNo = pmem_allocate(true);
			if(pde->ptFrameNo == 0)
				goto error;
			pde->present = true;
			pde->exists = true;
			/* writable because we want to be able to change PTE's in the PTE-area */
			/* is there another reason? :) */
			pde->writable = true;
			pde->notSuperVisor = !((flags & PG_SUPERVISOR) >> PG_SUPERVISOR_SHIFT);
			pts++;

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
				if(virt >= KERNEL_AREA) {
					pte->frameNumber = pmem_allocate(true);
					if(pte->frameNumber == 0)
						goto error;
				}
				else
					pte->frameNumber = thread_getFrame(t);
			}
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pte->frameNumber = *frames++ >> PAGE_SIZE_SHIFT;
				else
					pte->frameNumber = *frames++;
			}
		}

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

size_t paging_unmapFrom(tPageDir *pdir,uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts;
	spinlock_aquire(&pagingLock);
	pts = paging_doUnmapFrom(pdir,virt,count,freeFrames);
	spinlock_release(&pagingLock);
	return pts;
}

static size_t paging_doUnmapFrom(tPageDir *pdir,uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts = 0;
	tPageDir *cur = paging_getPageDir();
	uintptr_t ptables = paging_getPTables(pdir);
	size_t pti = PT_ENTRY_COUNT;
	size_t lastPti = PT_ENTRY_COUNT;
	sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	while(count-- > 0) {
		/* remove and free page-table, if necessary */
		pti = ADDR_TO_PDINDEX(virt);
		if(pti != lastPti) {
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				pts += paging_remEmptyPt(ptables,lastPti);
			lastPti = pti;
		}

		/* remove page and free if necessary */
		if(pte->present) {
			pte->present = false;
			if(freeFrames) {
				/* if its in user-space, its not critical */
				pmem_free(pte->frameNumber,virt >= KERNEL_AREA);
			}
		}
		pte->exists = false;

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

size_t paging_getPTableCount(tPageDir *pdir) {
	size_t i,count = 0;
	uintptr_t ptables;
	sPDEntry *pdirAddr;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(pdir);
	pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdirAddr[i].present)
			count++;
	}
	spinlock_release(&pagingLock);
	return count;
}

void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir *pdir) {
	size_t i,j;
	uintptr_t ptables;
	sPDEntry *pdirAddr;
	spinlock_aquire(&pagingLock);
	ptables = paging_getPTables(pdir);
	/* note that we release the lock here because prf_sprintf() might cause the cache-module to
	 * allocate more memory and use paging to map it. therefore, we would cause a deadlock if
	 * we use don't release it here.
	 * I think, in this case we can do it without lock because the only things that could happen
	 * are a delivery of wrong information to the user and a segfault, which would kill the
	 * process. Of course, only the process that is reading the information could cause these
	 * problems. Therefore, its not really a bad thing. */
	spinlock_release(&pagingLock);
	pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
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

static tPageDir *paging_getPageDir(void) {
	return proc_getPageDir();
}

static uintptr_t paging_getPTables(tPageDir *pdir) {
	tPageDir *cur = paging_getPageDir();
	sPDEntry *pde;
	if(pdir == cur)
		return MAPPED_PTS_START;
	if(pdir == cur->other) {
		/* do we have to refresh the mapping? */
		if(pdir->lastChange <= cur->otherUpdate)
			return TMPMAP_PTS_START;
	}
	/* map page-tables to other area */
	pde = ((sPDEntry*)PAGE_DIR_AREA) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	pde->present = true;
	pde->exists = true;
	pde->notSuperVisor = false;
	pde->ptFrameNo = pdir->own >> PAGE_SIZE_SHIFT;
	pde->writable = true;
	/* we want to access the whole page-table */
	paging_flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	cur->other = pdir;
	cur->otherUpdate = cpu_rdtsc();
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
	pmem_free(pde->ptFrameNo,true);
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

size_t paging_dbg_getPageCount(void) {
	size_t i,x,count = 0;
	sPTEntry *pagetable;
	sPDEntry *pdir = (sPDEntry*)PAGE_DIR_AREA;
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
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

void paging_printPageOf(tPageDir *pdir,uintptr_t virt) {
	/* don't use locking here; its only used in the debugging-console anyway. its better without
	 * locking to be able to view these things even if somebody currently holds the lock (e.g.
	 * because a panic occurred during that operation) */
	tPageDir *cur = proc_getPageDir();
	tPageDir *old = cur->other;
	uintptr_t ptables;
	sPDEntry *pdirAddr;
	ptables = paging_getPTables(pdir);
	pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	if(pdirAddr[ADDR_TO_PDINDEX(virt)].present) {
		sPTEntry *page = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
		vid_printf("Page @ %p: ",virt);
		paging_printPage(page);
		vid_printf("\n");
	}
	/* restore the old mapping, if necessary */
	if(cur->other != old)
		paging_getPTables(old);
}

void paging_printCur(uint parts) {
	paging_printPDir(paging_getPageDir(),parts);
}

void paging_printPDir(tPageDir *pdir,uint parts) {
	size_t i;
	tPageDir *cur = proc_getPageDir();
	tPageDir *old = cur->other;
	uintptr_t ptables;
	sPDEntry *pdirAddr;
	ptables = paging_getPTables(pdir);
	pdirAddr = (sPDEntry*)PAGEDIR(ptables);
	vid_printf("page-dir @ %p:\n",pdirAddr);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(!pdirAddr[i].present)
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
			paging_printPageTable(ptables,i,pdirAddr + i);
		}
	}
	vid_printf("\n");
	if(cur->other != old)
		paging_getPTables(old);
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
