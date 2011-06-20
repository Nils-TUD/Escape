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
#include <sys/mem/kheap.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/printf.h>
#include <string.h>
#include <assert.h>
#include <errors.h>

/*
 * PTE:
 * +-------+-+-------------------------------------------+----------+-+-+-+
 * |       |e|                 frameNumber               |     n    |r|w|x|
 * +-------+-+-------------------------------------------+----------+-+-+-+
 * 63     57 56                                          13         3     0
 *
 * PTP:
 * +-+---------------------------------------------------+----------+-+-+-+
 * |1|                   ptframeNumber                   |     n    | ign |
 * +-+---------------------------------------------------+----------+-+-+-+
 * 63                                                    13         3     0
 */

/* to shift a flag down to the first bit */
#define PG_PRESENT_SHIFT			0
#define PG_WRITABLE_SHIFT			1
#define PG_EXECUTABLE_SHIFT			2

/* pte fields */
#define PTE_EXISTS					(1UL << 56)
#define PTE_READABLE				(1UL << 2)
#define PTE_WRITABLE				(1UL << 1)
#define PTE_EXECUTABLE				(1UL << 0)
#define PTE_FRAMENO(pte)			(((pte) >> PAGE_SIZE_SHIFT) & 0x7FFFFFFFFFF)
#define PTE_FRAMENO_MASK			0x00FFFFFFFFFFE000
#define PTE_NMASK					0x0000000000001FF8

#define PAGE_NO(virt)				(((virt) & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT)
#define SEGSIZE(rV,i)				((i) == 0 ? 0 : (((rV) >> (64 - (i) * 4)) & 0xF))

static uint64_t *paging_getPTOf(tPageDir pdir,uintptr_t virt,bool create,size_t *createdPts);
static uint64_t paging_getPTEOf(tPageDir pdir,uintptr_t virt);
static size_t paging_removePts(tPageDir pdir,uint64_t pageNo,uint64_t c,ulong level,ulong depth);
static size_t paging_remEmptyPts(tPageDir pdir,uintptr_t virt);
static void paging_tcRemPT(tPageDir pdir,uintptr_t virt);
extern void tc_update(uint64_t key);
extern void paging_setrV(uint64_t rv);
static void paging_dbg_printPageTable(ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent);
static void paging_dbg_printPage(uint64_t pte);

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
	uint64_t pte,*pt = NULL;
	/* calc start and end */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	ulong pageNo = PAGE_NO(virt);
	while(virt < end) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			pt = paging_getPTOf(context,virt,true,NULL);
			if(pt == NULL)
				return false;
		}

		pte = pt[pageNo % PT_ENTRY_COUNT];
		if(!(pte & PTE_EXISTS))
			return false;
		if(!(pte & PTE_READABLE)) {
			/* we have to handle the page-fault here */
			if(!vmm_pagefault(virt))
				return false;
		}
		virt += PAGE_SIZE;
		pageNo++;
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
	uint64_t pte,*pt = NULL;
	/* calc start and end */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	ulong pageNo = PAGE_NO(virt);
	while(virt < end) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			pt = paging_getPTOf(context,virt,true,NULL);
			if(pt == NULL)
				return false;
		}

		pte = pt[pageNo % PT_ENTRY_COUNT];
		if(!(pte & PTE_EXISTS))
			return false;
		if((pte & (PTE_READABLE | PTE_WRITABLE)) != (PTE_READABLE | PTE_WRITABLE)) {
			if(!vmm_pagefault(virt))
				return false;
		}
		virt += PAGE_SIZE;
		pageNo++;
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
	ssize_t frmCount = 0;

	/* frames needed:
	 * 	- root location
	 * 	- kernel stack
	 */
	if(pmem_getFreeFrames(MM_DEF) < SEGMENT_COUNT * PTS_PER_SEGMENT + 1)
		return ERR_NOT_ENOUGH_MEM;

	/* allocate context */
	sContext *con = kheap_alloc(sizeof(sContext));
	if(con == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* allocate root-location */
	uintptr_t rootLoc;
	ssize_t res = pmem_allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1);
	frmCount += SEGMENT_COUNT * PTS_PER_SEGMENT;
	rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAPPED_SPACE;
	/* clear */
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* init context */
	con->addrSpace = aspace_alloc();
	con->ptables = 0;
	con->rV = context->rV & 0xFFFFFF0000000000;
	con->rV |= (rootLoc & ~DIR_MAPPED_SPACE) | (con->addrSpace->no << 3);

	/* allocate and clear kernel-stack */
	*stackFrame = pmem_allocate();
	frmCount++;
	memclear((void*)(DIR_MAPPED_SPACE | (*stackFrame * PAGE_SIZE)),PAGE_SIZE);
	*pdir = con;
	return frmCount;
}

sAllocStats paging_destroyPDir(tPageDir pdir) {
	sAllocStats stats;
	assert(pdir != context);
	/* don't free kernel-stack-frame; its done in thread_kill() */
	/* free page-dir */
	pmem_freeContiguous((pdir->rV >> PAGE_SIZE_SHIFT) & 0x7FFFFFF,SEGMENT_COUNT * PTS_PER_SEGMENT);
	stats.ptables += SEGMENT_COUNT * PTS_PER_SEGMENT;
	kheap_free(pdir);
	return stats;
}

bool paging_isPresent(tPageDir pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	return pte & PTE_EXISTS;
}

tFrameNo paging_getFrameNo(tPageDir pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	assert(pte & PTE_EXISTS);
	return PTE_FRAMENO(pte);
}

sAllocStats paging_clonePages(tPageDir src,tPageDir dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	assert(src != dst && (src == context || dst == context));
	sAllocStats stats = {0,0};
	ulong pageNo = PAGE_NO(virtSrc);
	uint64_t pte,*pt = NULL;
	while(count-- > 0) {
		sAllocStats mstats;
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			pt = paging_getPTOf(src,virtSrc,false,NULL);
			assert(pt != NULL);
		}
		pte = pt[pageNo % PT_ENTRY_COUNT];
		tFrameNo *frames = NULL;
		uint flags = 0;
		if(pte & PTE_READABLE)
			flags |= PG_PRESENT;
		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (share || !(pte & PTE_READABLE)))
			flags |= PG_WRITABLE;
		if(pte & PTE_EXECUTABLE)
			flags |= PG_EXECUTABLE;
		if(share || (pte & PTE_READABLE)) {
			flags |= PG_ADDR_TO_FRAME;
			frames = (tFrameNo*)&pte;
		}
		mstats = paging_mapTo(dst,virtDst,frames,1,flags);
		if(flags & PG_PRESENT)
			stats.frames++;
		stats.ptables += mstats.ptables;
		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_READABLE))
			paging_mapTo(src,virtSrc,NULL,1,flags | PG_KEEPFRM);
		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		pageNo++;
	}
	return stats;
}

sAllocStats paging_map(uintptr_t virt,const tFrameNo *frames,size_t count,uint flags) {
	return paging_mapTo(context,virt,frames,count,flags);
}

sAllocStats paging_mapTo(tPageDir pdir,uintptr_t virt,const tFrameNo *frames,size_t count,
		uint flags) {
	sAllocStats stats = {0,0};
	ulong pageNo = PAGE_NO(virt);
	uint64_t key,pteFlags = 0;
	uint64_t pteAttr = 0;
	uint64_t pte,oldFrame,*pt = NULL;

	virt &= ~(PAGE_SIZE - 1);
	if(flags & PG_PRESENT)
		pteFlags |= PTE_READABLE;
	if((flags & PG_PRESENT) && (flags & PG_WRITABLE))
		pteFlags |= PTE_WRITABLE;
	if((flags & PG_PRESENT) && (flags & PG_EXECUTABLE))
		pteFlags |= PTE_EXECUTABLE;
	pteAttr = pteFlags | (pdir->addrSpace->no << 3);
	key = virt | pteAttr;
	pteAttr |= PTE_EXISTS;

	while(count-- > 0) {
		/* get page-table */
		if(!pt || (pageNo % PT_ENTRY_COUNT) == 0) {
			size_t ptables = 0;
			pt = paging_getPTOf(pdir,virt,true,&ptables);
			assert(pt != NULL);
			stats.ptables += ptables;
			pdir->ptables += ptables;
		}

		oldFrame = pt[pageNo % PT_ENTRY_COUNT] & PTE_FRAMENO_MASK;
		pte = pteAttr;
		if(flags & PG_KEEPFRM)
			pte |= oldFrame;
		else if(flags & PG_PRESENT) {
			if(frames == NULL) {
				pte |= pmem_allocate() << PAGE_SIZE_SHIFT;
				stats.frames++;
			}
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pte |= *frames++ & PTE_FRAMENO_MASK;
				else
					pte |= *frames++ << PAGE_SIZE_SHIFT;
			}
		}
		pt[pageNo % PT_ENTRY_COUNT] = pte;

		/* update entries in TCs; protection-flags might have changed */
		/* we have to do that for not running processes as well, since their entry might still
		 * be in the TCs */
		/* note: if the frame has changed, we have to remove the entry instead of replacing the
		 * protection-flags */
		if(oldFrame == (pte & PTE_FRAMENO_MASK))
			tc_update(key);
		else
			tc_update(key & ~(PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE));

		/* to next page */
		virt += PAGE_SIZE;
		key += PAGE_SIZE;
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
	ulong pageNo = PAGE_NO(virt);
	uint64_t pte,*pt = NULL;
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
		pte = pt[pageNo % PT_ENTRY_COUNT];
		if(freeFrames && (pte & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE))) {
			if(freeFrames) {
				pmem_free(PTE_FRAMENO(pte));
				stats.frames++;
			}
		}
		pt[pageNo % PT_ENTRY_COUNT] = 0;

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

static uint64_t *paging_getPTOf(tPageDir pdir,uintptr_t virt,bool create,size_t *createdPts) {
	ulong j,i = virt >> 61;
	ulong size1 = SEGSIZE(pdir->rV,i + 1);
	ulong size2 = SEGSIZE(pdir->rV,i);
	uint64_t c,pageNo = PAGE_NO(virt);
	uint64_t limit = 1UL << (10 * (size1 - size2));
	if(size1 < size2 || pageNo >= limit)
		return NULL;

	j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = PAGE_NO(virt);
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
	return (uint64_t*)(DIR_MAPPED_SPACE | (c << 13));
}

static uint64_t paging_getPTEOf(tPageDir pdir,uintptr_t virt) {
	uint64_t pte = 0;
	uint64_t *pt = paging_getPTOf(pdir,virt,false,NULL);
	if(pt)
		return pt[PAGE_NO(virt) % PT_ENTRY_COUNT];
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
			uint64_t *ptp = (uint64_t*)(DIR_MAPPED_SPACE | (c << 13));
			for(i = 0; i < PT_ENTRY_COUNT; i++) {
				if(*ptp != 0) {
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
		uint64_t *pte = (uint64_t*)(DIR_MAPPED_SPACE | (c << 13));
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(*pte & PTE_EXISTS) {
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
	uint64_t pageNo = PAGE_NO(virt);
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

void paging_dbg_printPDir(tPageDir pdir,uint parts) {
	UNUSED(parts);
	size_t i,j;
	uintptr_t root = DIR_MAPPED_SPACE | (pdir->rV & 0xFFFFFFE000);
	/* go through all page-tables in the root-location */
	vid_printf("root-location @ %p [n=%X]:\n",root,(pdir->rV & 0x1FF8) >> 3);
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
	uint64_t *pte;
	if(level > 0) {
		/* page-table with PTPs */
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0) {
				vid_printf("%*sPTP%Sd[%Sd]=%PX n=%X (VM: %p - %p)\n",indent * 2,"",level,i,
						(pt[i] & ~DIR_MAPPED_SPACE) / PAGE_SIZE,(pt[i] & PTE_NMASK) >> 3,
						addr,addr + PAGE_SIZE * (1UL << (10 * level)) - 1);
				paging_dbg_printPageTable(seg,addr,
						(uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1,indent + 1);
			}
			addr += PAGE_SIZE * (1UL << (10 * level));
		}
	}
	else {
		/* page-table with PTEs */
		pte = (uint64_t*)pt;
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & PTE_EXISTS) {
				vid_printf("%*s%Sx: ",indent * 2,"",i);
				paging_dbg_printPage(pte[i]);
				vid_printf(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_dbg_printPage(uint64_t pte) {
	if(pte & PTE_EXISTS) {
		vid_printf("f=%PX n=%X [%c%c%c]",PTE_FRAMENO(pte),(pte & PTE_NMASK) >> 3,
				(pte & PTE_READABLE) ? 'r' : '-',
				(pte & PTE_WRITABLE) ? 'w' : '-',
				(pte & PTE_EXECUTABLE) ? 'x' : '-');
	}
	else {
		vid_printf("-");
	}
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void paging_dbg_printPageOf(tPageDir pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	if(pte & PTE_EXISTS) {
		vid_printf("Page @ %p: ",virt);
		paging_dbg_printPage(pte);
		vid_printf("\n");
	}
}

void paging_dbg_printCur(uint parts) {
	paging_dbg_printPDir(context,parts);
}

static size_t paging_dbg_getPageCountOf(uint64_t *pt,size_t level) {
	size_t i,count = 0;
	uint64_t *pte;
	if(level > 0) {
		/* page-table with PTPs */
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0)
				count += paging_dbg_getPageCountOf((uint64_t*)(pt[i] & 0xFFFFFFFFFFFFE000),level - 1);
		}
	}
	else {
		/* page-table with PTEs */
		pte = (uint64_t*)pt;
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i] & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE))
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

#endif
