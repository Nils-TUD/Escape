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

static sPTE *paging_getPTOf(tPageDir pdir,uintptr_t virt,bool create,size_t *createdPts);
static sPTE paging_getPTEOf(tPageDir pdir,uintptr_t virt);
static size_t paging_removePts(tPageDir pdir,uint64_t pageNo,uint64_t c,ulong level,ulong depth);
static size_t paging_remEmptyPts(tPageDir pdir,uintptr_t virt);
static void paging_tcRemPT(tPageDir pdir,uintptr_t virt);
extern void tc_update(uint64_t key);
extern void paging_setrV(uint64_t rv);

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
	if((res = pmem_allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1)) < 0)
		util_panic("Not enough contiguous memory for the root-location of the first process");
	rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAPPED_SPACE;
	/* clear */
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* create context for the first process */
	firstCon.addrSpace = aspace_alloc();
	/* set value for rV: b1 = 2, b2 = 4, b3 = 6, b4 = 0, page-size = 2^13 */
	firstCon.rV = 0x24600DUL << 40 | (rootLoc & ~DIR_MAPPED_SPACE) | (firstCon.addrSpace->no << 3);
	context = &firstCon;
	/* enable paging */
	paging_setrV(firstCon.rV);
}

tPageDir paging_getCur(void) {
	return context;
}

void paging_setCur(tPageDir pdir) {
	context = pdir;
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
			if(!vmm_pagefault(virt))
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
			if(!vmm_pagefault(virt))
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

ssize_t paging_cloneKernelspace(tFrameNo *stackFrame,tPageDir *pdir) {
#if 0
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

	/* allocate root-location */
	uintptr_t rootLoc;
	ssize_t res = pmem_allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1);
	rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAPPED_SPACE;
	/* clear */
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* build context */
	sContext *con = kheap_alloc(sizeof(sContext));
	if(con == NULL)
		return ERR_NOT_ENOUGH_MEM;
	con->addrSpace = aspace_alloc();
	con->ptables = 0;
	con->rV = context->rV & 0xFFFFFF0000000000;
	con->rV |= (rootLoc & ~DIR_MAPPED_SPACE) | (con->addrSpace->no << 3);

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
#endif
	return 0;
}

sAllocStats paging_destroyPDir(tPageDir pdir) {
	sAllocStats stats;
#if 0
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
#endif
	return stats;
}

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
			size_t ptables = 0;
			pt = paging_getPTOf(pdir,virt,true,&ptables);
			assert(pt != NULL);
			stats.ptables += ptables;
			pdir->ptables += ptables;
		}

		/* setup page */
		pte = pt + (pageNo % PT_ENTRY_COUNT);
		/* ignore already present entries */
		pte->executable = (flags & PG_EXECUTABLE) >> PG_EXECUTABLE_SHIFT;
		pte->readable = (flags & PG_PRESENT) >> PG_READABLE_SHIFT;
		pte->writable = (flags & PG_WRITABLE) >> PG_WRITABLE_SHIFT;
		pte->addressSpaceNumber = pdir->addrSpace->no;
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

sAllocStats paging_unmap(uintptr_t virt,size_t count,bool freeFrames) {
	return paging_unmapFrom(context,virt,count,freeFrames);
}

sAllocStats paging_unmapFrom(tPageDir pdir,uintptr_t virt,size_t count,bool freeFrames) {
	sAllocStats stats = {0,0};
	size_t ptables;
	ulong pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	sPTE *pte,*pt = NULL;
	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			/* not the first one? then check if its empty */
			if(pt) {
				ptables = paging_remEmptyPts(pdir,virt - PAGE_SIZE);
				stats.ptables += ptables;
				pdir->ptables -= ptables;
			}
			pt = paging_getPTOf(pdir,virt,false,NULL);
			assert(pt != NULL);
		}

		/* remove page and free if necessary */
		pte = pt + (pageNo % PT_ENTRY_COUNT);
		if(pte->readable | pte->writable | pte->executable) {
			pte->readable = false;
			pte->writable = false;
			pte->executable = false;
			if(freeFrames) {
				pmem_free(pte->frameNumber);
				stats.frames++;
			}
		}
		pte->exists = false;

		/* to next page */
		virt += PAGE_SIZE;
		pageNo++;
	}
	/* check if the last changed pagetable is empty (pt is NULL if no pages have been unmapped) */
	if(pt) {
		ptables = paging_remEmptyPts(pdir,virt - PAGE_SIZE);
		stats.ptables += ptables;
		pdir->ptables -= ptables;
	}
	return stats;
}

static sPTE *paging_getPTOf(tPageDir pdir,uintptr_t virt,bool create,size_t *createdPts) {
	ulong j,i = virt >> 61;
	ulong size1 = SEGSIZE(pdir->rV,i + 1);
	ulong size2 = SEGSIZE(pdir->rV,i);
	uint64_t c,pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	uint64_t limit = 1UL << (10 * (size1 - size2));
	if(size1 < size2 || pageNo >= limit)
		return NULL;

	j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	c = ((pdir->rV & 0xFFFFFFFFFF) >> 13) + size2 + j;
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
			if(createdPts)
				(*createdPts)++;
		}
		c = (ptp & ~DIR_MAPPED_SPACE) >> 13;
	}
	return (sPTE*)(DIR_MAPPED_SPACE | (c << 13));
}

static sPTE paging_getPTEOf(tPageDir pdir,uintptr_t virt) {
	sPTE pte = {0,0,0,0,0,0};
	ulong pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	sPTE *pt = paging_getPTOf(pdir,virt,false,NULL);
	if(pt)
		return pt[pageNo % PT_ENTRY_COUNT];
	return pte;
}

static size_t paging_removePts(tPageDir pdir,uint64_t pageNo,uint64_t c,ulong level,ulong depth) {
	size_t i;
	bool empty = true;
	size_t count = 0;
	/* page-table with PTPs? */
	if(level > 0) {
		ulong ax = (pageNo >> (10 * level)) & 0x3FF;
		uint64_t *ptpAddr = (uint64_t*)(DIR_MAPPED_SPACE | ((c << 13) + (ax << 3)));
		count = paging_removePts(pdir,pageNo,(*ptpAddr & ~DIR_MAPPED_SPACE) >> 13,level - 1,depth + 1);
		/* if count is equal to level, we know that all deeper page-tables have been free'd.
		 * therefore, we can remove the entry in our page-table */
		if(count == level)
			*ptpAddr = 0;
		/* don't free frames in the root-location */
		if(depth > 0) {
			/* now go through our page-table and check whether there still are used entries */
			sPTP *ptp = (sPTP*)(DIR_MAPPED_SPACE | (c << 13));
			for(i = 0; i < PT_ENTRY_COUNT; i++) {
				if(ptp->ptFrameNo != 0) {
					empty = false;
					break;
				}
				ptp++;
			}
			/* free the frame, if the pt is empty */
			if(empty) {
				pmem_free(c);
				count++;
			}
		}
	}
	else if(depth > 0) {
		/* go through our page-table and check whether there still are used entries */
		sPTE *pte = (sPTE*)(DIR_MAPPED_SPACE | (c << 13));
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte->exists) {
				empty = false;
				break;
			}
			pte++;
		}
		/* free the frame, if the pt is empty */
		if(empty) {
			/* remove all pages in that page-table from the TCs */
			paging_tcRemPT(pdir,pageNo * PAGE_SIZE);
			pmem_free(c);
			count++;
		}
	}
	return count;
}

static size_t paging_remEmptyPts(tPageDir pdir,uintptr_t virt) {
	ulong i = virt >> 61;
	uint64_t pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	int j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	uint64_t c = ((pdir->rV & 0xFFFFFFFFFF) >> 13) + SEGSIZE(pdir->rV,i) + j;
	return paging_removePts(pdir,pageNo,c,j,0);
}

static void paging_tcRemPT(tPageDir pdir,uintptr_t virt) {
	size_t i;
	uint64_t key = (virt & ~(PAGE_SIZE - 1)) | (pdir->addrSpace->no << 3);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		tc_update(key);
		key += PAGE_SIZE;
	}
}

size_t paging_getPTableCount(tPageDir pdir) {
	return pdir->ptables;
}

static sStringBuffer *strBuf = NULL;

static void paging_sprintfPrint(char c) {
	prf_sprintf(strBuf,"%c",c);
}

void paging_sprintfVirtMem(sStringBuffer *buf,tPageDir pdir) {
	strBuf = buf;
	vid_setPrintFunc(paging_sprintfPrint);
	paging_dbg_printPDir(pdir,0);
	vid_unsetPrintFunc();
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

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

static size_t paging_dbg_getPageCountOf(uint64_t *pt,size_t level) {
	size_t i,count = 0;
	sPTE *pte;
	if(level > 0) {
		/* page-table with PTPs */
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0)
				count += paging_dbg_getPageCountOf((uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1);
		}
	}
	else {
		/* page-table with PTEs */
		pte = (sPTE*)pt;
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].readable || pte[i].writable || pte[i].executable)
				count++;
		}
	}
	return count;
}

size_t paging_dbg_getPageCount(void) {
	size_t i,j,count = 0;
	uintptr_t root = DIR_MAPPED_SPACE | (context->rV & 0xFFFFFFE000);
	for(i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(context->rV,i + 1) - SEGSIZE(context->rV,i);
		for(j = 0; j < segSize; j++) {
			count += paging_dbg_getPageCountOf((uint64_t*)root,j);
			root += PAGE_SIZE;
		}
	}
	return count;
}

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
	for(i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(context->rV,i + 1) - SEGSIZE(context->rV,i);
		uintptr_t addr = (i << 61);
		vid_printf("segment %Su:\n",i);
		for(j = 0; j < segSize; j++) {
			paging_dbg_printPageTable(i,addr,(uint64_t*)root,j,1);
			addr = (i << 61) | (PAGE_SIZE * (1UL << (10 * (j + 1))));
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
