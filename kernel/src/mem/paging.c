/**
 * $Id$
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

#include <common.h>
#include <machine/intrpt.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <mem/kheap.h>
#include <mem/text.h>
#include <task/proc.h>
#include <task/thread.h>
#include <util.h>
#include <video.h>
#include <sllist.h>
#include <string.h>
#include <assert.h>
#include <asprintf.h>

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + (((u32)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + \
		(((u32)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000)
 * Note that this does not work anymore as soon as the GDT is "corrected" and paging enabled! */
#define VIRT2PHYS(addr) ((u32)(addr) + 0x40000000)

/* copy-on-write */
typedef struct {
	u32 frameNumber;
	sProc *proc;
} sCOW;

/**
 * Assembler routine to enable paging
 */
extern void paging_enable(void);

/**
 * Flushes the whole page-table including the page in the mapped page-table-area
 *
 * @param virt a virtual address in the page-table
 */
static void paging_flushPageTable(u32 virt);

/**
 * Maps the page-directory of the given process at PAGE_DIR_TMP_AREA and the page-tables
 * at TMPMAP_PTS_START
 *
 * @param p the process
 */
static void paging_mapForeignPageDir(sProc *p);

/**
 * Unmaps the page-tables that have been mapped via paging_mapForeignPageDir()
 */
static void paging_unmapForeignPageDir(void);

/**
 * paging_map() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 * @param mappingArea the address of the mapping area to use
 */
static void paging_mapIntern(u32 pageDir,u32 mappingArea,u32 virt,u32 *frames,u32 count,u8 flags,
		bool force);

/**
 * paging_unmap() for internal usages
 *
 * @param p the process (needed for remCOW)
 * @param mappingArea the address of the mapping area to use
 */
static void paging_unmapIntern(sProc *p,u32 mappingArea,u32 virt,u32 count,bool freeFrames,
		bool remCOW);

/**
 * paging_unmapPageTables() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 */
static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count);

/**
 * Helper function to put the given frames for the given new process and the current one
 * in the copy-on-write list and mark the pages for the parent as copy-on-write if not
 * already done.
 *
 * @panic not enough mem for linked-list nodes and entries
 *
 * @param virt the virtual address to map
 * @param pte the page-table-entries
 * @param count the number of pages to map
 * @param newProc the new process
 */
static void paging_setCOW(u32 virt,sPTEntry *pte,u32 count,sProc *newProc);

/**
 * Removes the given frame-number for the given process from COW
 *
 * @param p the process
 * @param frameNumber the frame-number
 */
static void paging_remFromCow(sProc *p,u32 frameNumber);

#if DEBUGGING
static void paging_dbg_printPageDir(u32 mappingArea,u32 pageDirArea,u8 parts);
static void paging_dbg_printPageTable(u32 mappingArea,u32 no,sPDEntry *pde);
static void paging_dbg_printPage(sPTEntry *page);
#endif

/* the page-directory for process 0 */
static sPDEntry proc0PD[PAGE_SIZE / sizeof(sPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-tables for process 0 (two because our mm-stack may get large if we have a lot physmem) */
static sPTEntry proc0PT1[PAGE_SIZE / sizeof(sPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));
static sPTEntry proc0PT2[PAGE_SIZE / sizeof(sPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

/**
 * A list which contains each frame for each process that is marked as copy-on-write.
 * If a process causes a page-fault we will remove it from the list. If there is no other
 * entry for the frame in the list, the process can keep the frame, otherwise it is copied.
 */
static sSLList *cowFrames = NULL;

void paging_init(void) {
	sPDEntry *pd,*pde;
	sPTEntry *pt;
	u32 j,i,vaddr,addr,end;
	sPTEntry *pts[] = {proc0PT1,proc0PT2};

	/* note that we assume here that the kernel including mm-stack is not larger than 2
	 * complete page-tables (8MiB)! */

	/* map 2 page-tables at 0xC0000000 */
	vaddr = KERNEL_AREA_V_ADDR;
	addr = KERNEL_AREA_P_ADDR;
	pd = (sPDEntry*)VIRT2PHYS(proc0PD);
	for(j = 0; j < 2; j++) {
		pt = (sPTEntry*)VIRT2PHYS(pts[j]);

		/* map one page-table */
		end = addr + (PT_ENTRY_COUNT) * PAGE_SIZE;
		for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
			/* build page-table entry */
			pts[j][i].frameNumber = (u32)addr >> PAGE_SIZE_SHIFT;
			pts[j][i].present = true;
			pts[j][i].writable = true;
		}

		/* insert page-table in the page-directory */
		pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(vaddr));
		pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
		pde->present = true;
		pde->writable = true;

		/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
		pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(vaddr - KERNEL_AREA_V_ADDR));
		pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
		pde->present = true;
		pde->writable = true;
		vaddr += PT_ENTRY_COUNT * PAGE_SIZE;
	}

	/* put the page-directory in the last page-dir-slot */
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptFrameNo = (u32)pd >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* now set page-dir and enable paging */
	paging_exchangePDir((u32)pd);
	paging_enable();
}

void paging_mapHigherHalf(void) {
	u32 addr,end;
	sPDEntry *pde;
	/* insert all page-tables for 0xC0800000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_AREA_V_ADDR + (PAGE_SIZE * PT_ENTRY_COUNT * 2);
	end = KERNEL_STACK;
	/*end = TMPMAP_PTS_START;*/
	/*end = KERNEL_HEAP_START + KERNEL_HEAP_SIZE;*/
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		u32 frame = mm_allocateFrame(MM_DEF);
		pde->ptFrameNo = frame;
		pde->present = true;
		pde->writable = true;
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
	proc0PD[1].present = false;
	paging_flushTLB();
}

sPDEntry *paging_getProc0PD(void) {
	return proc0PD;
}

bool paging_isMapped(u32 virt) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	return pt->present;
}

bool paging_isRangeUserReadable(u32 virt,u32 count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > KERNEL_AREA_V_ADDR || virt + count < virt)
		return false;

	return paging_isRangeReadable(virt,count);
}

bool paging_isRangeReadable(u32 virt,u32 count) {
	sPTEntry *pt;
	sPDEntry *pd;
	u32 end;
	/* calc start and end pt */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	while(virt < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virt);
		if(!pd->present)
			return false;
		if(!pt->present)
			return false;
		virt += PAGE_SIZE;
		pt++;
	}
	return true;
}

bool paging_isRangeUserWritable(u32 virt,u32 count) {
	/* kernel area? (be carefull with overflows!) */
	if(virt + count > KERNEL_AREA_V_ADDR || virt + count < virt)
		return false;

	return paging_isRangeWritable(virt,count);
}

bool paging_isRangeWritable(u32 virt,u32 count) {
	sPTEntry *pt;
	sPDEntry *pd;
	u32 end;
	/* calc start and end pt */
	end = (virt + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virt &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	while(virt < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virt);
		if(!pd->present)
			return false;
		if(!pt->present)
			return false;
		if(!pt->writable) {
			/* we have to handle copy-on-write here manually because the kernel can write
			 * to the page anyway */
			if(pt->copyOnWrite)
				paging_handlePageFault(virt);
			else
				return false;
		}
		virt += PAGE_SIZE;
		pt++;
	}
	return true;
}

u32 paging_getFrameOf(u32 virt) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(virt);
	if(pt->present == 0)
		return 0;
	return pt->frameNumber;
}

u32 paging_countFramesForMap(u32 virt,u32 count) {
	/* we need at least <count> frames */
	u32 res = count;
	/* signed is better here :) */
	s32 c = count;
	sPDEntry *pd;
	while(c > 0) {
		/* page-table not present yet? */
		pd = (sPDEntry*)(PAGE_DIR_AREA + ADDR_TO_PDINDEX(virt) * sizeof(sPDEntry));
		if(!pd->present) {
			res++;
		}

		/* advance to next page-table */
		c -= PT_ENTRY_COUNT;
		virt += PAGE_SIZE * PT_ENTRY_COUNT;
	}
	return res;
}

void paging_getFrameNos(u32 *nos,u32 addr,u32 size) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(addr);
	u32 pcount = BYTES_2_PAGES((addr & (PAGE_SIZE - 1)) + size);
	while(pcount-- > 0) {
		*nos++ = pt->frameNumber;
		pt++;
	}
}

void *paging_mapAreaOf(sProc *p,u32 addr,u32 size) {
	u32 count;
	count = BYTES_2_PAGES((addr & (PAGE_SIZE - 1)) + size);
	vassert(p != NULL && p->pid != INVALID_PID,"Invalid process %x\n",p);
	vassert(count <= TEMP_MAP_AREA_SIZE / PAGE_SIZE,"Temp-map-area too small for %d pages",count);

	paging_mapForeignPageDir(p);
	paging_mapIntern(PAGE_DIR_AREA,MAPPED_PTS_START,TEMP_MAP_AREA,
			(u32*)ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,addr),count,
			PG_WRITABLE | PG_SUPERVISOR | PG_ADDR_TO_FRAME,true);
	paging_unmapForeignPageDir();
	return (void*)(TEMP_MAP_AREA + (addr & (PAGE_SIZE - 1)));
}

void paging_unmapArea(u32 addr,u32 size) {
	sProc *p = proc_getRunning();
	u32 count = BYTES_2_PAGES((addr & (PAGE_SIZE - 1)) + size);
	paging_unmapIntern(p,MAPPED_PTS_START,TEMP_MAP_AREA,count,false,false);
}

void paging_getPagesOf(sProc *p,u32 srcAddr,u32 dstAddr,u32 count,u8 flags) {
	vassert(p != NULL && p->pid != INVALID_PID,"Invalid process %x\n",p);

	paging_mapForeignPageDir(p);
	/* now copy pages */
	paging_mapIntern(PAGE_DIR_AREA,MAPPED_PTS_START,dstAddr,
			(u32*)ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,srcAddr),count,flags | PG_ADDR_TO_FRAME,true);
	paging_unmapForeignPageDir();
}

void paging_remPagesOf(sProc *p,u32 addr,u32 count) {
	vassert(p != NULL && p->pid != INVALID_PID,"Invalid process %x\n",p);

	paging_mapForeignPageDir(p);
	paging_unmapIntern(p,TMPMAP_PTS_START,addr,count,true,true);
	paging_unmapForeignPageDir();
}

void paging_map(u32 virt,u32 *frames,u32 count,u8 flags,bool force) {
	paging_mapIntern(PAGE_DIR_AREA,MAPPED_PTS_START,virt,frames,count,flags,force);
}

static void paging_mapIntern(u32 pageDir,u32 mappingArea,u32 virt,u32 *frames,u32 count,u8 flags,
		bool force) {
	u32 frame;
	sPDEntry *pd;
	sPTEntry *pt;

	vassert(pageDir == PAGE_DIR_AREA || pageDir == PAGE_DIR_TMP_AREA,"pageDir invalid");
	vassert(mappingArea == MAPPED_PTS_START || mappingArea == TMPMAP_PTS_START,"mappingArea invalid");
	vassert(!(flags & ~(PG_WRITABLE | PG_SUPERVISOR | PG_COPYONWRITE | PG_ADDR_TO_FRAME |
			PG_NOFREE | PG_INHERIT)),"flags contain invalid bits");
	vassert(force == true || force == false,"force invalid");

	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		pd = (sPDEntry*)pageDir + ADDR_TO_PDINDEX(virt);
		/* page table not present? */
		if(!pd->present) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				util_panic("Not enough memory");
			}

			pd->ptFrameNo = frame;
			pd->present = true;
			/* writable because we want to be able to change PTE's in the PTE-area */
			/* is there another reason? :) */
			pd->writable = true;
			pd->notSuperVisor = (flags & PG_SUPERVISOR) == 0 ? true : false;

			paging_flushPageTable(virt);
			/* clear frame (ensure that we start at the beginning of the frame) */
			memclear((void*)ADDR_TO_MAPPED_CUSTOM(mappingArea,
					virt & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),PAGE_SIZE);
		}

		/* setup page */
		pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virt);
		/* ignore already present entries */
		if(force || !pt->present) {
			pt->present = true;
			pt->dirty = false;
			if(flags & PG_INHERIT) {
				pt->noFree = ((sPTEntry*)frames)->noFree;
				if(!pt->noFree)
					pt->copyOnWrite = (flags & PG_COPYONWRITE) ? true : false;
			}
			else {
				pt->noFree = (flags & PG_NOFREE) ? true : false;
				pt->copyOnWrite = (flags & PG_COPYONWRITE) ? true : false;
			}
			pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0 ? true : false;
			pt->writable = (flags & PG_WRITABLE) ? true : false;

			/* set frame-number */
			if(frames == NULL)
				pt->frameNumber = mm_allocateFrame(MM_DEF);
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pt->frameNumber = *frames++ >> PAGE_SIZE_SHIFT;
				else
					pt->frameNumber = *frames++;
			}

			/* invalidate TLB-entry */
			if(pageDir == PAGE_DIR_AREA)
				paging_flushAddr(virt);
		}

		/* to next page */
		virt += PAGE_SIZE;
	}
}

void paging_unmap(u32 virt,u32 count,bool freeFrames,bool remCOW) {
	paging_unmapIntern(proc_getRunning(),MAPPED_PTS_START,virt,count,freeFrames,remCOW);
}

void paging_unmapPageTables(u32 start,u32 count) {
	paging_unmapPageTablesIntern(PAGE_DIR_AREA,start,count);
}

u32 paging_clonePageDir(u32 *stackFrame,sProc *newProc) {
	u32 x,pdirFrame,frameCount,kheapCount;
	u32 tPages,dPages,sPages,tsPages;
	sPDEntry *pd,*npd,*tpd;
	sPTEntry *pt;
	sProc *p;
	sThread *curThread = thread_getRunning();

	vassert(stackFrame != NULL,"stackFrame == NULL");
	vassert(newProc != NULL,"newProc == NULL");

	DBG_PGCLONEPD(vid_printf(">>===== paging_clonePageDir(newPid=%d) =====\n",newPid));

	/* frames needed:
	 * 	- page directory
	 * 	- kernel-stack page-table
	 * 	- kernel stack
	 *  - page-tables for text+data
	 *  - page-tables for stack
	 * The frames for the page-content is not yet needed since we're using copy-on-write!
	 */
	p = curThread->proc;
	tPages = p->textPages;
	dPages = p->dataPages;
	sPages = curThread->ustackPages;
	frameCount = 3 + PAGES_TO_PTS(tPages + dPages) + PAGES_TO_PTS(sPages);
	/* worstcase heap-usage. NOTE THAT THIS ASSUMES A BIT ABOUT THE INTERNAL STRUCTURE OF SLL! */
	kheapCount = (dPages + sPages) * (sizeof(sCOW) + sizeof(sSLNode)) * 2;
	/* TODO finish! */
	if(mm_getFreeFrmCount(MM_DEF) < frameCount/* || kheap_getFreeMem() < kheapCount*/) {
		DBG_PGCLONEPD(vid_printf("Not enough free frames!\n"));
		return 0;
	}

	/* we need a new page-directory */
	pdirFrame = mm_allocateFrame(MM_DEF);
	DBG_PGCLONEPD(vid_printf("Got page-dir-frame %x\n",pdirFrame));

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	paging_map(TEMP_MAP_PAGE,&pdirFrame,1,PG_WRITABLE | PG_SUPERVISOR,true);

	pd = (sPDEntry*)PAGE_DIR_AREA;
	npd = (sPDEntry*)TEMP_MAP_PAGE;

	/* copy old page-dir to new one */
	DBG_PGCLONEPD(vid_printf("Copying old page-dir to new one (%x)\n",npd));
	/* clear user-space page-tables */
	memclear(npd,ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) * sizeof(sPDEntry));
	/* copy kernel-space page-tables*/
	/* TODO we don't need to copy the last few pts */
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)) * sizeof(sPDEntry));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	
	/* get new page-table for the kernel-stack-area and the stack itself */
	DBG_PGCLONEPD(vid_printf("Create kernel-stack\n"));
	tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK);
	tpd->ptFrameNo = mm_allocateFrame(MM_DEF);
	tpd->present = true;
	tpd->writable = true;
	/* clear the page-table */
	memclear((void*)ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,
			KERNEL_STACK & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),PAGE_SIZE);

	/* create stack-page */
	pt = (sPTEntry*)(TMPMAP_PTS_START + (KERNEL_STACK / PT_ENTRY_COUNT));
	pt->frameNumber = mm_allocateFrame(MM_DEF);
	*stackFrame = pt->frameNumber;
	pt->present = true;
	pt->writable = true;

	/* map pages for text to the frames of the old process */
	DBG_PGCLONEPD(vid_printf("Mapping %d text-pages (shared)\n",tPages));
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,
			0,(u32*)ADDR_TO_MAPPED(0),tPages,PG_ADDR_TO_FRAME,true);

	/* map pages for data. we will copy the data with copyonwrite. */
	DBG_PGCLONEPD(vid_printf("Mapping %d data-pages (copy-on-write)\n",dPages));
	x = tPages * PAGE_SIZE;
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,
			(u32*)ADDR_TO_MAPPED(x),dPages,PG_COPYONWRITE | PG_ADDR_TO_FRAME | PG_INHERIT,true);
	/* mark as copy-on-write for the current and new process */
	paging_setCOW(x,(sPTEntry*)ADDR_TO_MAPPED(x),dPages,newProc);

	/* we're cloning just the current thread */
	/* create user-stack */
	tsPages = curThread->ustackPages;
	x = curThread->ustackBegin - tsPages * PAGE_SIZE;
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,
				(u32*)ADDR_TO_MAPPED(x),tsPages,PG_COPYONWRITE | PG_ADDR_TO_FRAME | PG_INHERIT,true);
	paging_setCOW(x,(sPTEntry*)ADDR_TO_MAPPED(x),tsPages,newProc);

	/* unmap stuff */
	DBG_PGCLONEPD(vid_printf("Removing temp mappings\n"));
	paging_unmap(TEMP_MAP_PAGE,1,false,false);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;

	/* one final flush to ensure everything is correct */
	paging_flushTLB();

	DBG_PGCLONEPD(vid_printf("<<===== paging_clonePageDir() =====\n"));

	return pdirFrame;
}

void paging_destroyStacks(sThread *t) {
	u32 tsPages;
	paging_mapForeignPageDir(t->proc);

	/* free user-stack */
	tsPages = t->ustackPages;
	paging_unmapIntern(t->proc,TMPMAP_PTS_START,t->ustackBegin - tsPages * PAGE_SIZE,tsPages,true,true);
	/* free kernel-stack */
	mm_freeFrame(t->kstackFrame,MM_DEF);

	paging_unmapForeignPageDir();
}

void paging_destroyPageDir(sProc *p) {
	u32 ustackBegin;
	u32 ustackEnd;
	sSLNode *tn;
	vassert(p != NULL,"p == NULL");

	/* map page-dir and page-tables */
	paging_mapForeignPageDir(p);

	/* free text; free frames if text_free() returns true */
	paging_unmapIntern(p,TMPMAP_PTS_START,0,p->textPages,text_free(p->text,p->pid),false);

	/* free data-pages */
	paging_unmapIntern(p,TMPMAP_PTS_START,p->textPages * PAGE_SIZE,p->dataPages,true,true);

	/* free text- and data page-tables */
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,0,PAGES_TO_PTS(p->textPages + p->dataPages));

	/* we don't know which stacks are used and which not. therefore we have to search for the
	 * first and last stack. */
	ustackBegin = KERNEL_AREA_V_ADDR;
	ustackEnd = KERNEL_AREA_V_ADDR;
	for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
		sThread *t = (sThread*)tn->data;
		/* free user-stack */
		u32 tsPages = t->ustackPages;
		u32 tsEnd = t->ustackBegin - tsPages * PAGE_SIZE;
		paging_unmapIntern(p,TMPMAP_PTS_START,tsEnd,tsPages,true,true);

		/* free kernel-stack */
		mm_freeFrame(t->kstackFrame,MM_DEF);

		/* determine end of the stack for freeing page-tables */
		/* the start is always the same because we are destroying the page-tables just if
		 * the process is destroyed */
		if(tsEnd < ustackEnd)
			ustackEnd = tsEnd;
	}

	/* free page-tables for user- and kernel-stack */
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,
			ADDR_TO_PDINDEX(ustackEnd),PAGES_TO_PTS((ustackBegin - ustackEnd) / PAGE_SIZE));
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,ADDR_TO_PDINDEX(KERNEL_STACK),1);

	/* unmap stuff & free page-dir */
	paging_unmapForeignPageDir();
	mm_freeFrame(p->physPDirAddr >> PAGE_SIZE_SHIFT,MM_DEF);
}

static void paging_flushPageTable(u32 virt) {
	u32 end;
	/* to beginning of page-table */
	virt &= ~(PT_ENTRY_COUNT * PAGE_SIZE - 1);
	end = virt + PT_ENTRY_COUNT * PAGE_SIZE;
	/* flush page-table-entries in mapped area */
	paging_flushAddr(ADDR_TO_MAPPED(virt));
	/* flush pages in the page-table */
	while(virt < end) {
		paging_flushAddr(virt);
		virt += PAGE_SIZE;
	}
}

static void paging_mapForeignPageDir(sProc *p) {
	sPDEntry *pd,*ppd;
	/* map page-dir of process */
	paging_map(TEMP_MAP_PAGE,&(p->physPDirAddr),1,
			PG_SUPERVISOR | PG_WRITABLE | PG_ADDR_TO_FRAME,true);
	/* map page-tables */
	pd = (sPDEntry*)PAGE_DIR_AREA;
	ppd = (sPDEntry*)TEMP_MAP_PAGE;
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = ppd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	paging_unmap(TEMP_MAP_PAGE,1,false,false);
	/* we want to access the whole page-table */
	paging_flushTLB();
}

static void paging_unmapForeignPageDir(void) {
	/* unmap stuff & free page-dir */
	sPDEntry *pd = (sPDEntry*)PAGE_DIR_AREA;
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;
	paging_flushTLB();
}

static void paging_unmapIntern(sProc *p,u32 mappingArea,u32 virt,u32 count,bool freeFrames,
		bool remCOW) {
	sPTEntry *pte = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virt);

	vassert(mappingArea == MAPPED_PTS_START || mappingArea == TMPMAP_PTS_START,"mappingArea invalid");
	vassert(freeFrames == true || freeFrames == false,"freeFrames invalid");

	while(count-- > 0) {
		/* remove and free, if desired */
		if(pte->present) {
			/* we don't want to free copy-on-write pages and not frames in front of the kernel
			 * because they may be mapped more than once and will never be free'd */
			if(freeFrames && !pte->noFree) {
				if(pte->copyOnWrite && remCOW)
					paging_remFromCow(p,pte->frameNumber);
				else if(!pte->copyOnWrite)
					mm_freeFrame(pte->frameNumber,MM_DEF);
			}
			pte->present = false;

			/* invalidate TLB-entry */
			if(mappingArea == MAPPED_PTS_START) {
				/* FIXME I think a flushAddr() should be enough here. But somehow, we need
				 * a complete flush. I don't know why :/ */
				/*paging_flushAddr(virt);*/
				paging_flushTLB();
			}
		}

		/* to next page */
		pte++;
		virt += PAGE_SIZE;
	}
}

static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count) {
	sPDEntry *pde;
	pde = (sPDEntry*)pageDir + start;

	vassert(pageDir == PAGE_DIR_AREA || pageDir == PAGE_DIR_TMP_AREA,"pageDir invalid");
	vassert(start < PT_ENTRY_COUNT,"start >= PT_ENTRY_COUNT");
	vassert(count < PT_ENTRY_COUNT,"count >= PT_ENTRY_COUNT");

	while(count-- > 0) {
		if(pde->present) {
			pde->present = 0;
			mm_freeFrame(pde->ptFrameNo,MM_DEF);
		}
		pde++;
	}

	if(pageDir == PAGE_DIR_AREA)
		paging_flushTLB();
}

void paging_initCOWList(void) {
	cowFrames = sll_create();
	if(cowFrames == NULL)
		util_panic("Not enough mem for copy-on-write-list!");
}

bool paging_handlePageFault(u32 address) {
	sSLNode *n,*ln;
	sCOW *cow;
	sSLNode *ourCOW,*ourPrevCOW;
	bool foundOther;
	u32 frameNumber;
	sProc *cp;
	sPDEntry *pd;
	sPTEntry *pt;

	/* check if the page exists */
	pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(address);
	pt = (sPTEntry*)ADDR_TO_MAPPED(address);
	if(!pd->present || !pt->copyOnWrite || !pt->present)
		return false;

	/* search through the copy-on-write-list wether there is another one who wants to get
	 * the frame */
	cp = proc_getRunning();
	ourCOW = NULL;
	ourPrevCOW = NULL;
	foundOther = false;
	frameNumber = pt->frameNumber;
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ln = n, n = n->next) {
		cow = (sCOW*)n->data;
		if(cow->frameNumber == frameNumber) {
			if(cow->proc == cp) {
				ourCOW = n;
				ourPrevCOW = ln;
			}
			else
				foundOther = true;
			/* if we have both, we can stop here */
			if(ourCOW && foundOther)
				break;
		}
	}

	/* should never happen */
	if(ourCOW == NULL)
		util_panic("No COW entry for process %d and address 0x%x",cp->pid,address);

	/* remove our from list and adjust pte */
	kheap_free(ourCOW->data);
	sll_removeNode(cowFrames,ourCOW,ourPrevCOW);
	pt->copyOnWrite = false;
	pt->writable = true;
	/* if there is another process who wants to get the frame, we make a copy for us */
	/* otherwise we keep the frame for ourself */
	if(foundOther)
		pt->frameNumber = mm_allocateFrame(MM_DEF);
	paging_flushAddr(address);

	/* copy? */
	if(foundOther) {
		/* map the frame to copy it */
		paging_map(TEMP_MAP_PAGE,&frameNumber,1,PG_WRITABLE | PG_SUPERVISOR,true);

		/* copy content */
		memcpy((void*)(address & ~(PAGE_SIZE - 1)),(void*)TEMP_MAP_PAGE,PAGE_SIZE);

		/* unmap */
		paging_unmap(TEMP_MAP_PAGE,1,false,false);
	}

	return true;
}

static void paging_setCOW(u32 virt,sPTEntry *pte,u32 count,sProc *newProc) {
	sPTEntry *ownPt;
	sCOW *cow;
	sProc *p = proc_getRunning();

	virt &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		if(!pte->noFree) {
			/* build cow-entry for child */
			cow = (sCOW*)kheap_alloc(sizeof(sCOW));
			if(cow == NULL)
				util_panic("Not enough mem for copy-on-write!");
			cow->frameNumber = pte->frameNumber;
			cow->proc = newProc;
			if(!sll_append(cowFrames,cow))
				util_panic("Not enough mem for copy-on-write!");

			/* build cow-entry for parent if not already done */
			ownPt = (sPTEntry*)ADDR_TO_MAPPED(virt);
			if(!ownPt->copyOnWrite) {
				cow = (sCOW*)kheap_alloc(sizeof(sCOW));
				if(cow == NULL)
					util_panic("Not enough mem for copy-on-write!");
				cow->frameNumber = pte->frameNumber;
				cow->proc = p;
				if(!sll_append(cowFrames,cow))
					util_panic("Not enough mem for copy-on-write!");

				ownPt->copyOnWrite = true;
				ownPt->writable = false;
				ownPt->dirty = false;
				paging_flushAddr(virt);
			}
		}

		pte++;
		virt += PAGE_SIZE;
	}
}

static void paging_remFromCow(sProc *p,u32 frameNumber) {
	sSLNode *n,*tn,*ln;
	sCOW *cow;
	bool foundOwn = false,foundOther = false;

	/* search for the frame in the COW-list */
	ln = NULL;
	for(n = sll_begin(cowFrames); n != NULL; ) {
		cow = (sCOW*)n->data;
		if(cow->proc == p && cow->frameNumber == frameNumber) {
			/* remove from COW-list */
			tn = n->next;
			kheap_free(cow);
			sll_removeNode(cowFrames,n,ln);
			n = tn;
			foundOwn = true;
			continue;
		}

		/* usage of other process? */
		if(cow->frameNumber == frameNumber)
			foundOther = true;
		/* we can stop if we have both */
		if(foundOther && foundOwn)
			break;
		ln = n;
		n = n->next;
	}

	/* if no other process uses this frame, we have to free it */
	if(!foundOther)
		mm_freeFrame(frameNumber,MM_DEF);
}

void paging_sprintfVirtMem(sStringBuffer *buf,sProc *p) {
	u32 i,j;
	sPDEntry *pagedir;
	paging_mapForeignPageDir(p);
	pagedir = (sPDEntry*)PAGE_DIR_TMP_AREA;
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR); i++) {
		if(pagedir[i].present) {
			u32 addr = PAGE_SIZE * PT_ENTRY_COUNT * i;
			sPTEntry *pte = (sPTEntry*)(TMPMAP_PTS_START + i * PAGE_SIZE);
			asprintf(buf,"PageTable 0x%x (VM: 0x%08x - 0x%08x)\n",i,addr,
					addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
			for(j = 0; j < PT_ENTRY_COUNT; j++) {
				if(pte[j].present) {
					sPTEntry *page = pte + j;
					asprintf(buf,"\tPage 0x%x: ",j);
					asprintf(buf,"frame=0x%x [%c%c%c%c%c] (VM: 0x%08x - 0x%08x)\n",
							page->frameNumber,page->notSuperVisor ? 'u' : 'k',
							page->writable ? 'w' : 'r',page->global ? 'g' : '-',
							page->copyOnWrite ? 'c' : '-',page->noFree ? 'n' : '-',
							addr,addr + PAGE_SIZE - 1);
				}
				addr += PAGE_SIZE;
			}
		}
	}
	paging_unmapForeignPageDir();
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void paging_dbg_printCOW(void) {
	sSLNode *n;
	sCOW *cow;
	vid_printf("COW-Frames:\n");
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		cow = (sCOW*)n->data;
		vid_printf("\tframe=0x%x, pid=%d, pcmd=%s\n",cow->frameNumber,cow->proc->pid,
				cow->proc->command);
	}
}

sPTEntry *paging_dbg_getPTEntry(u32 virt) {
	return (sPTEntry*)ADDR_TO_MAPPED(virt);
}

u32 paging_dbg_getPageCount(void) {
	u32 i,x,count = 0;
	sPTEntry *pagetable;
	sPDEntry *pdir = (sPDEntry*)PAGE_DIR_AREA;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pdir[i].present) {
			pagetable = (sPTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pagetable[x].present) {
					count++;
				}
			}
		}
	}
	return count;
}

bool paging_dbg_isPTEmpty(sPTEntry *pt) {
	u32 i;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pt->present) {
			return false;
		}
		pt++;
	}
	return true;
}

u32 paging_dbg_getPTEntryCount(sPTEntry *pt) {
	u32 i,count = 0;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pt->present) {
			count++;
		}
		pt++;
	}
	return count;
}

void paging_dbg_printOwnPageDir(u8 parts) {
	paging_dbg_printPageDir(MAPPED_PTS_START,PAGE_DIR_AREA,parts);
}

void paging_dbg_printPageDirOf(sProc *p,u8 parts) {
	paging_mapForeignPageDir(p);
	paging_dbg_printPageDir(TMPMAP_PTS_START,PAGE_DIR_TMP_AREA,parts);
	paging_unmapForeignPageDir();
}

static void paging_dbg_printPageDir(u32 mappingArea,u32 pageDirArea,u8 parts) {
	u32 i;
	sPDEntry *pagedir;
	pagedir = (sPDEntry*)pageDirArea;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pagedir[i].present) {
			if(parts == PD_PART_ALL ||
				(i < ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) && (parts & PD_PART_USER)) ||
				(i >= ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) && i < ADDR_TO_PDINDEX(KERNEL_HEAP_START)
						&& (parts & PD_PART_KERNEL)) ||
				(i >= ADDR_TO_PDINDEX(TEMP_MAP_AREA) && i < ADDR_TO_PDINDEX(TEMP_MAP_AREA + TEMP_MAP_AREA_SIZE)
						&& (parts & PD_PART_TEMPMAP)) ||
				(i >= ADDR_TO_PDINDEX(KERNEL_HEAP_START) && i < ADDR_TO_PDINDEX(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
						&& (parts & PD_PART_KHEAP)) ||
				(i >= ADDR_TO_PDINDEX(MAPPED_PTS_START) && (parts & PD_PART_PTBLS))) {
				paging_dbg_printPageTable(mappingArea,i,pagedir + i);
			}
		}
	}
	vid_printf("\n");
}

static void paging_dbg_printPageTable(u32 mappingArea,u32 no,sPDEntry *pde) {
	u32 i;
	u32 addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	sPTEntry *pte = (sPTEntry*)(mappingArea + no * PAGE_SIZE);
	vid_printf("\tpt 0x%x [frame 0x%x, %c%c] @ 0x%08x: (VM: 0x%08x - 0x%08x)\n",no,
			pde->ptFrameNo,pde->notSuperVisor ? 'u' : 'k',pde->writable ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].present) {
				vid_printf("\t\t0x%x: ",i);
				paging_dbg_printPage(pte + i);
				vid_printf(" (VM: 0x%08x - 0x%08x)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

static void paging_dbg_printPage(sPTEntry *page) {
	if(page->present) {
		vid_printf("r=0x%08x fr=0x%x [%c%c%c%c%c]",*(u32*)page,
				page->frameNumber,page->notSuperVisor ? 'u' : 'k',page->writable ? 'w' : 'r',
				page->global ? 'g' : '-',page->copyOnWrite ? 'c' : '-',
				page->noFree ? 'n' : '-');
	}
	else {
		vid_printf("-");
	}
}

#endif
