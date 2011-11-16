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
#include <sys/mem/vmm.h>
#include <sys/mem/cache.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/cpu.h>
#include <sys/util.h>
#include <sys/video.h>
#include <sys/printf.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

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

#define PAGE_NO(virt)				(((uintptr_t)(virt) & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT)
#define SEGSIZE(rV,i)				((i) == 0 ? 0 : (((rV) >> (64 - (i) * 4)) & 0xF))

static pagedir_t *paging_getPageDir(void);
static uint64_t *paging_getPTOf(const pagedir_t *pdir,uintptr_t virt,bool create,size_t *createdPts);
static uint64_t paging_getPTEOf(const pagedir_t *pdir,uintptr_t virt);
static size_t paging_removePts(pagedir_t *pdir,uint64_t pageNo,uint64_t c,ulong level,ulong depth);
static size_t paging_remEmptyPts(pagedir_t *pdir,uintptr_t virt);
static void paging_tcRemPT(pagedir_t *pdir,uintptr_t virt);
extern void tc_clear(void);
extern void tc_update(uint64_t key);
extern void paging_setrV(uint64_t rv);
static void paging_printPageTable(ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent);
static void paging_printPage(uint64_t pte);

static pagedir_t firstCon;

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
	firstCon.rv = 0x24600DUL << 40 | (rootLoc & ~DIR_MAPPED_SPACE) | (firstCon.addrSpace->no << 3);
	/* enable paging */
	paging_setrV(firstCon.rv);
}

void paging_setFirst(pagedir_t *pdir) {
	pdir->addrSpace = firstCon.addrSpace;
	pdir->rv = firstCon.rv;
	pdir->ptables = firstCon.ptables;
}

bool paging_isInUserSpace(uintptr_t virt,size_t count) {
	return virt + count <= DIR_MAPPED_SPACE && virt + count >= virt;
}

int paging_cloneKernelspace(pagedir_t *pdir) {
	pagedir_t *cur = paging_getPageDir();
	uintptr_t rootLoc;
	/* allocate root-location */
	ssize_t res = pmem_allocateContiguous(SEGMENT_COUNT * PTS_PER_SEGMENT,1);
	if(res < 0)
		return res;
	/* clear */
	rootLoc = (uintptr_t)(res * PAGE_SIZE) | DIR_MAPPED_SPACE;
	memclear((void*)rootLoc,PAGE_SIZE * SEGMENT_COUNT * PTS_PER_SEGMENT);

	/* init context */
	pdir->addrSpace = aspace_alloc();
	pdir->ptables = 0;
	pdir->rv = cur->rv & 0xFFFFFF0000000000;
	pdir->rv |= (rootLoc & ~DIR_MAPPED_SPACE) | (pdir->addrSpace->no << 3);
	return 0;
}

void paging_destroyPDir(pagedir_t *pdir) {
	assert(pdir != paging_getPageDir());
	/* free page-dir */
	pmem_freeContiguous((pdir->rv >> PAGE_SIZE_SHIFT) & 0x7FFFFFF,SEGMENT_COUNT * PTS_PER_SEGMENT);
	/* we have to ensure that no tc-entries of the current process are present. otherwise we could
	 * get the problem that the old process had a page that the new process with this
	 * address-space-number has not. if the entry of the old one is still in the tc, the new
	 * process could access it and write to that frame (which might be used for a different
	 * purpose now) */
	tc_clear();
}

bool paging_isPresent(pagedir_t *pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	return pte & PTE_EXISTS;
}

frameno_t paging_getFrameNo(pagedir_t *pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	assert(pte & PTE_EXISTS);
	return PTE_FRAMENO(pte);
}

uintptr_t paging_getAccess(frameno_t frame) {
	return frame * PAGE_SIZE | DIR_MAPPED_SPACE;
}

void paging_removeAccess(void) {
	/* nothing to do */
}

frameno_t paging_demandLoad(void *buffer,size_t loadCount,ulong regFlags) {
	/* copy into frame */
	sThread *t = thread_getRunning();
	frameno_t frame = thread_getFrame(t);
	uintptr_t addr = frame * PAGE_SIZE | DIR_MAPPED_SPACE;
	memcpy((void*)addr,buffer,loadCount);
	/* if its an executable region, we have to syncid the memory afterwards */
	if(regFlags & RF_EXECUTABLE)
		cpu_syncid(addr,addr + loadCount);
	return frame;
}

void paging_copyToFrame(frameno_t frame,const void *src) {
	memcpy((void*)(frame * PAGE_SIZE | DIR_MAPPED_SPACE),src,PAGE_SIZE);
}

void paging_copyFromFrame(frameno_t frame,void *dst) {
	memcpy(dst,(void*)(frame * PAGE_SIZE | DIR_MAPPED_SPACE),PAGE_SIZE);
}

void paging_copyToUser(void *dst,const void *src,size_t count) {
	uint64_t pte,*dpt = NULL;
	ulong dstPageNo = PAGE_NO(dst);
	pagedir_t *cur = paging_getPageDir();
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	uintptr_t addr = (uintptr_t)dst;
	while(count > 0) {
		if(!dpt || (dstPageNo % PT_ENTRY_COUNT) == 0) {
			dpt = paging_getPTOf(cur,addr,false,NULL);
			assert(dpt != NULL);
		}
		pte = dpt[dstPageNo % PT_ENTRY_COUNT];
		addr = ((pte & PTE_FRAMENO_MASK) | DIR_MAPPED_SPACE) + offset;

		size_t amount = MIN(PAGE_SIZE - offset,count);
		memcpy((void*)addr,src,amount);
		src = (const void*)((uintptr_t)src + amount);
		count -= amount;
		offset = 0;
		dstPageNo++;
	}
}

void paging_zeroToUser(void *dst,size_t count) {
	uint64_t pte,*dpt = NULL;
	ulong dstPageNo = PAGE_NO(dst);
	pagedir_t *cur = paging_getPageDir();
	uintptr_t offset = (uintptr_t)dst & (PAGE_SIZE - 1);
	uintptr_t addr = (uintptr_t)dst;
	while(count > 0) {
		if(!dpt || (dstPageNo % PT_ENTRY_COUNT) == 0) {
			dpt = paging_getPTOf(cur,addr,false,NULL);
			assert(dpt != NULL);
		}
		pte = dpt[dstPageNo % PT_ENTRY_COUNT];
		addr = ((pte & PTE_FRAMENO_MASK) | DIR_MAPPED_SPACE) + offset;

		size_t amount = MIN(PAGE_SIZE - offset,count);
		memclear((void*)addr,amount);
		count -= amount;
		offset = 0;
		dstPageNo++;
	}
}

ssize_t paging_clonePages(pagedir_t *src,pagedir_t *dst,uintptr_t virtSrc,uintptr_t virtDst,
		size_t count,bool share) {
	pagedir_t *cur = paging_getPageDir();
	sThread *t = thread_getRunning();
	ssize_t pts = 0;
	uintptr_t orgVirtSrc = virtSrc,orgVirtDst = virtDst;
	size_t orgCount = count;
	ulong srcPageNo = PAGE_NO(virtSrc);
	ulong dstPageNo = PAGE_NO(virtDst);
	uint64_t pte,*spt = NULL,*dpt = NULL;
	uint64_t dstAddrSpace = dst->addrSpace->no << 3;
	uint64_t dstKey = virtDst | dstAddrSpace;
	uint64_t srcKey = virtSrc | (src->addrSpace->no << 3);
	assert(src != dst && (src == cur || dst == cur));
	while(count > 0) {
		/* get src-page-table */
		if(!spt || (srcPageNo % PT_ENTRY_COUNT) == 0) {
			spt = paging_getPTOf(src,virtSrc,false,NULL);
			assert(spt != NULL);
		}
		/* get dest-page-table */
		if(!dpt || (dstPageNo % PT_ENTRY_COUNT) == 0) {
			size_t ptables = 0;
			dpt = paging_getPTOf(dst,virtDst,true,&ptables);
			if(!dpt)
				goto error;
			pts += ptables;
			dst->ptables += ptables;
		}
		pte = spt[srcPageNo % PT_ENTRY_COUNT];

		/* when shared, simply copy the flags; otherwise: if present, we use copy-on-write */
		if((pte & PTE_WRITABLE) && (!share && (pte & PTE_READABLE)))
			pte &= ~PTE_WRITABLE;
		if(!share && !(pte & PTE_READABLE)) {
			pte &= ~PTE_FRAMENO_MASK;
			pte |= thread_getFrame(t) << PAGE_SIZE_SHIFT;
		}
		pte &= ~PTE_NMASK;
		pte |= dstAddrSpace;
		dpt[dstPageNo % PT_ENTRY_COUNT] = pte;
		tc_update(dstKey);

		/* if copy-on-write should be used, mark it as readable for the current (parent), too */
		if(!share && (pte & PTE_READABLE)) {
			spt[srcPageNo % PT_ENTRY_COUNT] &= ~PTE_WRITABLE;
			tc_update(srcKey | (pte & (PTE_READABLE | PTE_EXECUTABLE)));
		}

		virtSrc += PAGE_SIZE;
		virtDst += PAGE_SIZE;
		srcKey += PAGE_SIZE;
		dstKey += PAGE_SIZE;
		srcPageNo++;
		dstPageNo++;
		count--;
	}
	return pts;

error:
	/* unmap from dest-pagedir; the frames are always owned by src */
	paging_unmapFrom(dst,orgVirtDst,orgCount - count,false);
	/* make the cow-pages writable again */
	spt = NULL;
	srcPageNo = PAGE_NO(orgVirtSrc);
	while(orgCount > count) {
		if(!spt || (srcPageNo % PT_ENTRY_COUNT) == 0) {
			spt = paging_getPTOf(src,orgVirtSrc,false,NULL);
			assert(spt != NULL);
		}
		pte = spt[srcPageNo % PT_ENTRY_COUNT];
		if(!share && (pte & PTE_READABLE)) {
			spt[srcPageNo % PT_ENTRY_COUNT] |= PTE_WRITABLE;
			tc_update(srcKey | (pte & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE)));
		}
		orgVirtSrc += PAGE_SIZE;
		srcPageNo++;
		orgCount--;
	}
	return -ENOMEM;
}

ssize_t paging_map(uintptr_t virt,const frameno_t *frames,size_t count,uint flags) {
	return paging_mapTo(paging_getPageDir(),virt,frames,count,flags);
}

ssize_t paging_mapTo(pagedir_t *pdir,uintptr_t virt,const frameno_t *frames,size_t count,
		uint flags) {
	ssize_t pts = 0;
	uintptr_t orgVirt = virt;
	size_t orgCount = count;
	ulong pageNo = PAGE_NO(virt);
	sThread *t = thread_getRunning();
	uint64_t key,pteFlags = 0;
	uint64_t pteAttr = 0;
	uint64_t pte,oldFrame,*pt = NULL;
	bool freeFrames;

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
			if(!pt)
				goto error;
			pts += ptables;
			pdir->ptables += ptables;
		}

		oldFrame = pt[pageNo % PT_ENTRY_COUNT] & PTE_FRAMENO_MASK;
		pte = pteAttr;
		if(flags & PG_KEEPFRM)
			pte |= oldFrame;
		else if(flags & PG_PRESENT) {
			if(frames == NULL) {
				/* we can't map anything for the kernel on mmix */
				pte |= thread_getFrame(t) << PAGE_SIZE_SHIFT;
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
	return pts;

error:
	freeFrames = frames == NULL && !(flags & PG_KEEPFRM) && (flags & PG_PRESENT);
	paging_unmapFrom(pdir,orgVirt,orgCount - count,freeFrames);
	return -ENOMEM;
}

size_t paging_unmap(uintptr_t virt,size_t count,bool freeFrames) {
	return paging_unmapFrom(paging_getPageDir(),virt,count,freeFrames);
}

size_t paging_unmapFrom(pagedir_t *pdir,uintptr_t virt,size_t count,bool freeFrames) {
	size_t pts = 0;
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
				pts += ptables;
				pdir->ptables -= ptables;
			}
			pt = paging_getPTOf(pdir,virt,false,NULL);
			assert(pt != NULL);
		}

		/* remove page and free if necessary */
		pte = pt[pageNo % PT_ENTRY_COUNT];
		if(freeFrames && (pte & (PTE_READABLE | PTE_WRITABLE | PTE_EXECUTABLE))) {
			if(freeFrames)
				pmem_free(PTE_FRAMENO(pte),FRM_KERNEL);
		}
		pt[pageNo % PT_ENTRY_COUNT] = 0;

		/* to next page */
		virt += PAGE_SIZE;
		pageNo++;
	}
	/* check if the last changed pagetable is empty (pt is NULL if no pages have been unmapped) */
	if(pt) {
		ptables = paging_remEmptyPts(pdir,virt - PAGE_SIZE);
		pts += ptables;
		pdir->ptables -= ptables;
	}
	return pts;
}

static pagedir_t *paging_getPageDir(void) {
	return proc_getPageDir();
}

static uint64_t *paging_getPTOf(const pagedir_t *pdir,uintptr_t virt,bool create,size_t *createdPts) {
	ulong j,i = virt >> 61;
	ulong size1 = SEGSIZE(pdir->rv,i + 1);
	ulong size2 = SEGSIZE(pdir->rv,i);
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
	c = ((pdir->rv & 0xFFFFFFFFFF) >> 13) + size2 + j;
	for(; j > 0; j--) {
		ulong ax = (pageNo >> (10 * j)) & 0x3FF;
		uint64_t *ptpAddr = (uint64_t*)(DIR_MAPPED_SPACE | ((c << 13) + (ax << 3)));
		uint64_t ptp = *ptpAddr;
		if(ptp == 0) {
			frameno_t frame;
			if(!create)
				return NULL;
			/* allocate page-table and clear it */
			frame = pmem_allocate(FRM_KERNEL);
			if(frame == 0)
				return NULL;
			*ptpAddr = DIR_MAPPED_SPACE | (frame * PAGE_SIZE);
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

static uint64_t paging_getPTEOf(const pagedir_t *pdir,uintptr_t virt) {
	uint64_t pte = 0;
	uint64_t *pt = paging_getPTOf(pdir,virt,false,NULL);
	if(pt)
		return pt[PAGE_NO(virt) % PT_ENTRY_COUNT];
	return pte;
}

static size_t paging_removePts(pagedir_t *pdir,uint64_t pageNo,uint64_t c,ulong level,ulong depth) {
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
				pmem_free(c,FRM_KERNEL);
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
			pmem_free(c,FRM_KERNEL);
			count++;
		}
	}
	return count;
}

static size_t paging_remEmptyPts(pagedir_t *pdir,uintptr_t virt) {
	ulong i = virt >> 61;
	uint64_t pageNo = PAGE_NO(virt);
	int j = 0;
	while(pageNo >= PT_ENTRY_COUNT) {
		j++;
		pageNo /= PT_ENTRY_COUNT;
	}
	pageNo = (virt & 0x1FFFFFFFFFFFFFFF) >> PAGE_SIZE_SHIFT;
	uint64_t c = ((pdir->rv & 0xFFFFFFFFFF) >> 13) + SEGSIZE(pdir->rv,i) + j;
	return paging_removePts(pdir,pageNo,c,j,0);
}

static void paging_tcRemPT(pagedir_t *pdir,uintptr_t virt) {
	size_t i;
	uint64_t key = (virt & ~(PAGE_SIZE - 1)) | (pdir->addrSpace->no << 3);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		tc_update(key);
		key += PAGE_SIZE;
	}
}

size_t paging_getPTableCount(pagedir_t *pdir) {
	return pdir->ptables;
}

static sStringBuffer *strBuf = NULL;

static void paging_sprintfPrint(char c) {
	prf_sprintf(strBuf,"%c",c);
}

void paging_sprintfVirtMem(sStringBuffer *buf,pagedir_t *pdir) {
	strBuf = buf;
	vid_setPrintFunc(paging_sprintfPrint);
	paging_printPDir(pdir,0);
	vid_unsetPrintFunc();
}

void paging_printPDir(pagedir_t *pdir,A_UNUSED uint parts) {
	size_t i,j;
	uintptr_t root = DIR_MAPPED_SPACE | (pdir->rv & 0xFFFFFFE000);
	/* go through all page-tables in the root-location */
	vid_printf("root-location @ %p [n=%X]:\n",root,(pdir->rv & 0x1FF8) >> 3);
	for(i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(pdir->rv,i + 1) - SEGSIZE(pdir->rv,i);
		uintptr_t addr = (i << 61);
		vid_printf("segment %zu:\n",i);
		for(j = 0; j < segSize; j++) {
			paging_printPageTable(i,addr,(uint64_t*)root,j,1);
			addr = (i << 61) | (PAGE_SIZE * (1UL << (10 * (j + 1))));
			root += PAGE_SIZE;
		}
	}
	vid_printf("\n");
}

static void paging_printPageTable(ulong seg,uintptr_t addr,uint64_t *pt,size_t level,ulong indent) {
	size_t i;
	uint64_t *pte;
	if(level > 0) {
		/* page-table with PTPs */
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pt[i] != 0) {
				vid_printf("%*sPTP%zd[%zd]=%PX n=%X (VM: %p - %p)\n",indent * 2,"",level,i,
						(pt[i] & ~DIR_MAPPED_SPACE) / PAGE_SIZE,(pt[i] & PTE_NMASK) >> 3,
						addr,addr + PAGE_SIZE * (1UL << (10 * level)) - 1);
				paging_printPageTable(seg,addr,
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
				vid_printf("%*s%zx: ",indent * 2,"",i);
				paging_printPage(pte[i]);
				vid_printf(" (VM: %p - %p)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_printPage(uint64_t pte) {
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

void paging_printPageOf(pagedir_t *pdir,uintptr_t virt) {
	uint64_t pte = paging_getPTEOf(pdir,virt);
	if(pte & PTE_EXISTS) {
		vid_printf("Page @ %p: ",virt);
		paging_printPage(pte);
		vid_printf("\n");
	}
}

void paging_printCur(uint parts) {
	paging_printPDir(paging_getPageDir(),parts);
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
	pagedir_t *cur = paging_getPageDir();
	uintptr_t root = DIR_MAPPED_SPACE | (cur->rv & 0xFFFFFFE000);
	for(i = 0; i < SEGMENT_COUNT; i++) {
		ulong segSize = SEGSIZE(cur->rv,i + 1) - SEGSIZE(cur->rv,i);
		for(j = 0; j < segSize; j++) {
			count += paging_dbg_getPageCountOf((uint64_t*)root,j);
			root += PAGE_SIZE;
		}
	}
	return count;
}
