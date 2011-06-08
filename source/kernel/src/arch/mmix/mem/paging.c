/**
 * $Id: paging.c 906 2011-06-04 15:45:33Z nasmussen $
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <sys/mem/vmm.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/printf.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

/* to shift a flag down to the first bit */
#define PG_EXECUTABLE_SHIFT			6
#define PG_WRITABLE_SHIFT			0
#define PG_READABLE_SHIFT			2

#define SEGSIZE(rV,i)				((i) == 0 ? 0 : (((rV) >> (64 - (i) * 4)) & 0xF))

/* represents a page-table-entry */
typedef struct {
	/* unused */
	uint64_t 				: 15,
	/* Indicates that this page exists (but must not necessarily be present) */
	exists					: 1,
	/* the physical address of the page, divided by the page-size */
	frameNumber				: 35,
	/* address-space-number; has to be equal in all PTEs, PTPs and in rV */
	addressSpaceNumber		: 10,
	/* permissions */
	readable				: 1,
	writable				: 1,
	executable				: 1;
} sPTE;

/* represents a page-table-pointer */
typedef struct {
	/* has to be set to 1 */
	uint64_t priv			: 1,
	/* the physical address of the page-table, divided by the page-size */
	ptFrameNo				: 50,
	/* address-space-number; has to be equal in all PTEs, PTPs and in rV */
	addressSpaceNumber		: 10,
	/* permissions */
							: 3;
} sPTP;


/**
 * Flushes the whole page-table including the page in the mapped page-table-area
 *
 * @param virt a virtual address in the page-table
 * @param ptables the page-table mapping-area
 */
static void paging_flushPageTable(uintptr_t virt,uintptr_t ptables);

static uintptr_t paging_getPTables(tPageDir pdir);
static size_t paging_remEmptyPt(tPageDir pdir,uintptr_t ptables,size_t pti);

#if DEBUGGING
static void paging_dbg_printPageTable(ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent);
static void paging_dbg_printPage(sPTE pte);
#endif

/* the current context with the address-space and the value of rV */
static tPageDir context = NULL;
static sContext firstCon;

void paging_init(void) {
	uintptr_t rootLoc;
	ssize_t res;
	/* set root-location of first process */
	if((res = pmem_allocateContiguous(8,1)) < 0)
		util_panic("Not enough contiguous memory for the root-location of the first process");
	rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAPPED_SPACE;
	/* clear */
	memclear((void*)rootLoc,PAGE_SIZE * 8);

	/* create context for the first process */
	firstCon.addrSpace = aspace_alloc();
	/* set value for rV: b1 = 4, b2 = 8, b3 = 1, b4 = 0, page-size = 2^13 */
	firstCon.rV = 0x48100DUL << 40 | (rootLoc & ~DIR_MAPPED_SPACE) | (firstCon.addrSpace->no << 3);
	context = &firstCon;
}

tPageDir paging_getCur(void) {
	return context;
}

void paging_setCur(tPageDir pdir) {
	context = pdir;
}

static sPTE *paging_getPTOf(tPageDir pdir,uintptr_t virt,bool create) {
	ulong i = virt >> 61;
	int size1 = SEGSIZE(pdir->rV,i + 1);
	int size2 = SEGSIZE(pdir->rV,i);
	uint64_t pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	uint64_t limit = 1UL << (10 * (size1 - size2));
	if(size1 < size2 || pageNo >= limit)
		return NULL;

	int j = 0;
	while(pageNo >= 1024) {
		j++;
		pageNo /= 1024;
	}
	pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	uint64_t c = ((pdir->rV & 0xFFFFFFFFFF) >> 13) + size2 + j;
	for(; j > 0; j--) {
		ulong ax = (pageNo >> (10 * j)) & 0x3FF;
		uint64_t *ptpAddr = (uint64_t*)(DIR_MAPPED_SPACE | ((c << 13) + (ax << 3)));
		uint64_t ptp = *ptpAddr;
		if(ptp == 0) {
			if(!create)
				return NULL;
			/* allocate page-table and clear it */
			*ptpAddr = DIR_MAPPED_SPACE | (pmem_allocate() * PAGE_SIZE);
			memclear((void*)*ptpAddr,PAGE_SIZE);
			/* put rV.n in that page-table */
			*ptpAddr |= pdir->addrSpace->no << 3;
			ptp = *ptpAddr;
		}
		c = (ptp & ~DIR_MAPPED_SPACE) >> 13;
	}
	return (sPTE*)(DIR_MAPPED_SPACE | (c << 13));
}

static sPTE paging_getPTEOf(tPageDir pdir,uintptr_t virt) {
	sPTE pte = {0,0,0,0,0,0};
	ulong pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	sPTE *pt = paging_getPTOf(pdir,virt,false);
	if(pt)
		return pt[pageNo % 1024];
	return pte;
}

/* TODO perhaps we should move isRange*() to vmm? */

bool paging_isRangeUserReadable(uintptr_t virt,size_t count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > DIR_MAPPED_SPACE || virt + count < virt)
		return false;

	return paging_isRangeReadable(virt,count);
}

bool paging_isRangeReadable(uintptr_t virt,size_t count) {
	uintptr_t end;
	/* calc start and end */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	while(virt < end) {
		sPTE pte = paging_getPTEOf(context,virt);
		if(!pte.exists)
			return false;
		if(!pte.readable) {
			/* we have to handle the page-fault here */
			/* TODO if(!vmm_pagefault(virt))*/
				return false;
		}
		virt += PAGE_SIZE;
	}
	return true;
}

bool paging_isRangeUserWritable(uintptr_t virt,size_t count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > DIR_MAPPED_SPACE || virt + count < virt)
		return false;

	return paging_isRangeWritable(virt,count);
}

bool paging_isRangeWritable(uintptr_t virt,size_t count) {
	uintptr_t end;
	/* calc start and end */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	while(virt < end) {
		sPTE pte = paging_getPTEOf(context,virt);
		if(!pte.exists)
			return false;
		if(!pte.readable || !pte.writable) {
			/* TODO if(!vmm_pagefault(virt))*/
				return false;
		}
		virt += PAGE_SIZE;
	}
	return true;
}

uintptr_t paging_mapToTemp(const tFrameNo *frames,size_t count) {
	/* TODO */
	if(count > 1)
		util_panic("Not possible with MMIX");
	return (*frames * PAGE_SIZE) | DIR_MAPPED_SPACE;
}

void paging_unmapFromTemp(size_t count) {
	UNUSED(count);
	/* nothing to do */
}

#if 0
ssize_t paging_cloneKernelspace(tFrameNo *stackFrame,tPageDir *pdir) {
	tFrameNo pdirFrame;
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
	pd = (sPDEntry*)PAGE_DIR_DIRMAP;
	npd = (sPDEntry*)((pdirFrame * PAGE_SIZE) | DIR_MAPPED_SPACE);

	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA) * sizeof(sPDEntry));
	/* copy kernel-space page-tables (beginning to temp-map-area, inclusive) */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA),
			(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE - KERNEL_AREA) /
				(PAGE_SIZE * PT_ENTRY_COUNT) * sizeof(sPDEntry));
	/* clear the rest */
	memclear(npd + ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)) * sizeof(sPDEntry));

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
	otherPDir = 0;
	pt = (sPTEntry*)((tpd->ptFrameNo * PAGE_SIZE) | DIR_MAPPED_SPACE);
	memclear(pt,PAGE_SIZE);

	/* create stack-page */
	pt += ADDR_TO_PTINDEX(KERNEL_STACK);
	pt->frameNumber = pmem_allocate();
	pt->present = true;
	pt->writable = true;
	pt->exists = true;
	frmCount++;

	*stackFrame = pt->frameNumber;
	*pdir = DIR_MAPPED_SPACE | (pdirFrame << PAGE_SIZE_SHIFT);
	return frmCount;
}

sAllocStats paging_destroyPDir(tPageDir pdir) {
	sAllocStats stats;
	sPDEntry *pde;
	assert(pdir != rootLoc);
	/* remove kernel-stack (don't free the frame; its done in thread_kill()) */
	stats = paging_unmapFrom(pdir,KERNEL_STACK,1,false);
	/* free page-table for kernel-stack */
	pde = (sPDEntry*)PAGE_DIR_DIRMAP_OF(pdir) + ADDR_TO_PDINDEX(KERNEL_STACK);
	pde->present = false;
	pde->exists = false;
	pmem_free(pde->ptFrameNo);
	stats.ptables++;
	/* free page-dir */
	pmem_free((pdir & ~DIR_MAPPED_SPACE) >> PAGE_SIZE_SHIFT);
	stats.ptables++;
	/* ensure that we don't use it again */
	otherPDir = 0;
	return stats;
}
#endif

bool paging_isPresent(tPageDir pdir,uintptr_t virt) {
	sPTE pte = paging_getPTEOf(pdir,virt);
	return pte.exists;
}

tFrameNo paging_getFrameNo(tPageDir pdir,uintptr_t virt) {
	sPTE pte = paging_getPTEOf(pdir,virt);
	assert(pte.exists);
	return pte.frameNumber;
}

sAllocStats paging_clonePages(tPageDir src,tPageDir dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	sAllocStats stats = {0,0};
	assert(src != dst && (src == context || dst == context));
	while(count-- > 0) {
		sAllocStats mstats;
		sPTE pte = paging_getPTEOf(src,virtSrc);
		tFrameNo *frames = NULL;
		uint flags = 0;
		if(pte.readable)
			flags |= PG_PRESENT;
		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if(pte.writable && (share || !pte.readable))
			flags |= PG_WRITABLE;
		if(pte.executable)
			flags |= PG_EXECUTABLE;
		if(share || pte.readable) {
			flags |= PG_ADDR_TO_FRAME;
			frames = (tFrameNo*)&pte;
		}
		mstats = paging_mapTo(dst,virtDst,frames,1,flags);
		if(flags & (PG_PRESENT | PG_EXECUTABLE))
			stats.frames++;
		stats.ptables += mstats.ptables;
		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && pte.readable)
			paging_mapTo(src,virtSrc,NULL,1,flags | PG_KEEPFRM);
		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
	}
	return stats;
}

sAllocStats paging_map(uintptr_t virt,const tFrameNo *frames,size_t count,uint flags) {
	return paging_mapTo(context,virt,frames,count,flags);
}

sAllocStats paging_mapTo(tPageDir pdir,uintptr_t virt,const tFrameNo *frames,size_t count,
		uint flags) {
	sAllocStats stats = {0,0};
	ulong pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	sPTE *pte,*pt = NULL;
	assert(!(flags & ~(PG_WRITABLE | PG_EXECUTABLE | PG_SUPERVISOR | PG_PRESENT | PG_ADDR_TO_FRAME |
			PG_GLOBAL | PG_KEEPFRM)));

	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			/* TODO stats for pts */
			pt = paging_getPTOf(pdir,virt,true);
			assert(pt != NULL);
		}

		/* setup page */
		pte = pt + (pageNo % PT_ENTRY_COUNT);
		/* ignore already present entries */
		pte->executable = (flags & PG_EXECUTABLE) >> PG_EXECUTABLE_SHIFT;
		pte->readable = (flags & PG_PRESENT) >> PG_READABLE_SHIFT;
		pte->writable = (flags & PG_WRITABLE) >> PG_WRITABLE_SHIFT;
		pte->exists = true;

		/* set frame-number */
		if(!(flags & PG_KEEPFRM) && (flags & (PG_PRESENT | PG_WRITABLE | PG_EXECUTABLE))) {
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

		/* to next page */
		virt += PAGE_SIZE;
		pageNo++;
	}
	return stats;
}

#if 0
sAllocStats paging_unmap(uintptr_t virt,size_t count,bool freeFrames) {
	return paging_unmapFrom(rootLoc,virt,count,freeFrames);
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
			if(lastPti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
				stats.ptables += paging_remEmptyPt(pdir,ptables,lastPti);
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
		if(pdir == rootLoc)
			tlb_remove(virt);

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
	/* check if the last changed pagetable is empty */
	if(pti != PT_ENTRY_COUNT && virt < KERNEL_AREA)
		stats.ptables += paging_remEmptyPt(pdir,ptables,pti);
	return stats;
}

static size_t paging_remEmptyPt(tPageDir pdir,uintptr_t ptables,size_t pti) {
	size_t i;
	sPDEntry *pde;
	uintptr_t virt = pti * PAGE_SIZE * PT_ENTRY_COUNT;
	sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pte[i].exists)
			return 0;
	}
	/* empty => can be removed */
	pde = (sPDEntry*)PAGE_DIR_DIRMAP_OF(pdir) + pti;
	pde->present = false;
	pde->exists = false;
	pmem_free(pde->ptFrameNo);
	if(ptables == MAPPED_PTS_START)
		paging_flushPageTable(virt,ptables);
	else
		tlb_remove((uintptr_t)pte);
	return 1;
}

size_t paging_getPTableCount(tPageDir pdir) {
	size_t i,count = 0;
	sPDEntry *pdirAddr = (sPDEntry*)PAGE_DIR_DIRMAP_OF(pdir);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA); i++) {
		if(pdirAddr[i].present)
			count++;
	}
	return count;
}

void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir pdir) {
	size_t i,j;
	uintptr_t ptables = paging_getPTables(pdir);
	sPDEntry *pdirAddr = (sPDEntry*)PAGE_DIR_DIRMAP_OF(pdir);
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
					prf_sprintf(buf,"frame 0x%05x [%c%c] (VM: 0x%08x - 0x%08x)\n",
							page->frameNumber,page->present ? 'p' : '-',
							page->writable ? 'w' : 'r',
							addr,addr + PAGE_SIZE - 1);
				}
				addr += PAGE_SIZE;
			}
		}
	}
}

#if 0
void paging_getFrameNos(tFrameNo *nos,uintptr_t addr,size_t size) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(addr);
	size_t pcount = BYTES_2_PAGES((addr & (PAGE_SIZE - 1)) + size);
	while(pcount-- > 0) {
		*nos++ = pt->frameNumber;
		pt++;
	}
}
#endif

static void paging_flushPageTable(uintptr_t virt,uintptr_t ptables) {
	int i;
	uintptr_t mapAddr = ADDR_TO_MAPPED_CUSTOM(ptables,virt);
	/* to beginning of page-table */
	virt &= ~(PT_ENTRY_COUNT * PAGE_SIZE - 1);
	for(i = TLB_FIXED; i < TLB_SIZE; i++) {
		uint entryHi,entryLo;
		tlb_get(i,&entryHi,&entryLo);
		/* affected by the page-table? */
		if((entryHi >= virt && entryHi < virt + PAGE_SIZE * PT_ENTRY_COUNT) || entryHi == mapAddr)
			tlb_set(i,DIR_MAPPED_SPACE,0);
	}
}

static uintptr_t paging_getPTables(tPageDir pdir) {
	sPDEntry *pde;
	if(pdir == rootLoc)
		return MAPPED_PTS_START;
	if(pdir == otherPDir)
		return TMPMAP_PTS_START;
	/* map page-tables to other area*/
	pde = ((sPDEntry*)PAGE_DIR_DIRMAP) + ADDR_TO_PDINDEX(TMPMAP_PTS_START);
	pde->present = true;
	pde->exists = true;
	pde->ptFrameNo = (pdir & ~DIR_MAPPED_SPACE) >> PAGE_SIZE_SHIFT;
	pde->writable = true;
	/* we want to access the whole page-table */
	paging_flushPageTable(TMPMAP_PTS_START,MAPPED_PTS_START);
	otherPDir = pdir;
	return TMPMAP_PTS_START;
}
#endif

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

#if 0
size_t paging_dbg_getPageCount(void) {
	size_t i,x,count = 0;
	sPTEntry *pagetable;
	sPDEntry *pdir = (sPDEntry*)PAGE_DIR_DIRMAP;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
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
#endif

void paging_dbg_printPageOf(tPageDir pdir,uintptr_t virt) {
	sPTE pte = paging_getPTEOf(pdir,virt);
	if(pte.exists) {
		vid_printf("Page @ %p: ",virt);
		paging_dbg_printPage(pte);
		vid_printf("\n");
	}
}

void paging_dbg_printCur(uint parts) {
	paging_dbg_printPDir(context,parts);
}

void paging_dbg_printPDir(tPageDir pdir,uint parts) {
	size_t i,j;
	uintptr_t root = DIR_MAPPED_SPACE | (pdir->rV & 0xFFFFFFE000);
	/* go through all page-tables in the root-location */
	vid_printf("root-location @ %p:\n",root);
	for(i = 0; i < 2; i++) {
		uintptr_t addr = (i << 61);
		vid_printf("segment %Su:\n",i);
		for(j = 0; j < 4; j++) {
			paging_dbg_printPageTable(i,addr,(uint64_t*)root,j,1);
			addr += PAGE_SIZE * (1UL << (10 * (j + 1)));
			root += PAGE_SIZE;
		}
	}
	vid_printf("\n");
}

static void paging_dbg_printPageTable(ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent) {
	size_t i;
	sPTE *pte;
	if(level > 0) {
		/* page-table with PTPs */
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0) {
				vid_printf("%*sPTP%Sd[%Sd]=%Px (VM: %p - %p)\n",indent * 2,"",level,i,
						(pt[i] & ~DIR_MAPPED_SPACE) / PAGE_SIZE,
						addr,addr + PAGE_SIZE * (1UL << (10 * level)) - 1);
				paging_dbg_printPageTable(seg,addr,
						(uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1,indent + 1);
			}
			addr += PAGE_SIZE * (1UL << (10 * level));
		}
	}
	else {
		/* page-table with PTEs */
		pte = (sPTE*)pt;
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].exists) {
				vid_printf("%*s%Sx: ",indent * 2,"",i);
				paging_dbg_printPage(pte[i]);
				vid_printf(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_dbg_printPage(sPTE pte) {
	if(pte.exists) {
		vid_printf("%x [%c%c%c]",pte.frameNumber,
				pte.readable ? 'r' : '-',
				pte.writable ? 'w' : '-',
				pte.executable ? 'x' : '-');
	}
	else {
		vid_printf("-");
	}
}

#endif
