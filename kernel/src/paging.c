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

#include <paging.h>
#include <util.h>
#include <proc.h>
#include <intrpt.h>
#include <kheap.h>
#include <video.h>
#include <sllist.h>
#include <string.h>
#include <assert.h>

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
 * Ensures that the current page-dir is mapped and can be accessed at PAGE_DIR_AREA
 */
static void paging_mapPageDir(void);

/**
 * paging_map() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 * @param mappingArea the address of the mapping area to use
 */
static void paging_mapIntern(u32 pageDir,u32 mappingArea,u32 virtual,u32 *frames,u32 count,u8 flags,
		bool force);

/**
 * paging_unmap() for internal usages
 *
 * @param p the process (needed for remCOW)
 * @param mappingArea the address of the mapping area to use
 */
static void paging_unmapIntern(sProc *p,u32 mappingArea,u32 virtual,u32 count,bool freeFrames,
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
 * @param virtual the virtual address to map
 * @param frames the frames array
 * @param count the number of pages to map
 * @param newProc the new process
 */
static void paging_setCOW(u32 virtual,u32 *frames,u32 count,sProc *newProc);

/**
 * Removes the given frame-number for the given process from COW
 *
 * @param p the process
 * @param frameNumber the frame-number
 */
static void paging_remFromCow(sProc *p,u32 frameNumber);

/* the page-directory for process 0 */
sPDEntry proc0PD[PAGE_SIZE / sizeof(sPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for process 0 */
sPTEntry proc0PT[PAGE_SIZE / sizeof(sPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

/**
 * A list which contains each frame for each process that is marked as copy-on-write.
 * If a process causes a page-fault we will remove it from the list. If there is no other
 * entry for the frame in the list, the process can keep the frame, otherwise it is copied.
 */
static sSLList *cowFrames = NULL;

void paging_init(void) {
	sPDEntry *pd,*pde;
	sPTEntry *pt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */

	pd = (sPDEntry*)VIRT2PHYS(proc0PD);
	pt = (sPTEntry*)VIRT2PHYS(proc0PT);

	/* map the first 4MiB at 0xC0000000 (exclude page-dir and temp page-dir) */
	addr = KERNEL_AREA_P_ADDR;
	end = KERNEL_AREA_P_ADDR + (PT_ENTRY_COUNT - 2) * PAGE_SIZE;
	for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
		/* build page-table entry */
		proc0PT[i].frameNumber = (u32)addr >> PAGE_SIZE_SHIFT;
		proc0PT[i].present = true;
		proc0PT[i].writable = true;
	}

	/* insert page-table in the page-directory */
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	pde = (sPDEntry*)proc0PD;
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* insert page-dir into page-table so that we can access our page-dir */
	pt = (sPTEntry*)(proc0PT + ADDR_TO_PTINDEX(PAGE_DIR_AREA));
	pt->frameNumber = (u32)pd >> PAGE_SIZE_SHIFT;
	pt->present = true;
	pt->writable = true;

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
	/* insert all page-tables for 0xC0400000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_AREA_V_ADDR + (PAGE_SIZE * PT_ENTRY_COUNT);
	end = TMPMAP_PTS_START;
	pde = (sPDEntry*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		u32 frame = mm_allocateFrame(MM_DEF);
		pde->ptFrameNo = frame;
		pde->present = true;
		pde->writable = true;
		/* clear */
		memset((void*)ADDR_TO_MAPPED(addr),0,PAGE_SIZE);
		/* to next */
		pde++;
		addr += PAGE_SIZE * PT_ENTRY_COUNT;
	}
}

void paging_initCOWList(void) {
	cowFrames = sll_create();
	if(cowFrames == NULL)
		util_panic("Not enough mem for copy-on-write-list!");
}

sPDEntry *paging_getProc0PD(void) {
	return proc0PD;
}

static void paging_mapPageDir(void) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(PAGE_DIR_AREA);
	sProc *p = proc_getRunning();
	u32 pdirFrame = (p->physPDirAddr >> PAGE_SIZE_SHIFT);
	/* not the current one? */
	if(pt->frameNumber != pdirFrame) {
		pt->frameNumber = pdirFrame;
		paging_flushAddr(PAGE_DIR_AREA);
	}
}

bool paging_isMapped(u32 virtual) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(virtual);
	return pt->present;
}

bool paging_isRangeUserReadable(u32 virtual,u32 count) {
	/* kernel area? (be carefull with overflows!) */
	if(virtual > KERNEL_AREA_V_ADDR || virtual + count > KERNEL_AREA_V_ADDR)
		return false;

	return paging_isRangeReadable(virtual,count);
}

bool paging_isRangeReadable(u32 virtual,u32 count) {
	sPTEntry *pt;
	sPDEntry *pd;
	u32 end;
	/* calc start and end pt */
	paging_mapPageDir();
	end = (virtual + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virtual &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virtual);
	while(virtual < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual);
		if(!pd->present)
			return false;
		if(!pt->present)
			return false;
		virtual += PAGE_SIZE;
		pt++;
	}
	return true;
}

bool paging_isRangeUserWritable(u32 virtual,u32 count) {
	/* kernel area? (be carefull with overflows!) */
	if(virtual > KERNEL_AREA_V_ADDR || virtual + count > KERNEL_AREA_V_ADDR)
		return false;

	return paging_isRangeWritable(virtual,count);
}

bool paging_isRangeWritable(u32 virtual,u32 count) {
	sPTEntry *pt;
	sPDEntry *pd;
	u32 end;
	/* calc start and end pt */
	paging_mapPageDir();
	end = (virtual + count + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	virtual &= ~(PAGE_SIZE - 1);
	pt = (sPTEntry*)ADDR_TO_MAPPED(virtual);
	while(virtual < end) {
		/* check page-table first */
		pd = (sPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual);
		if(!pd->present)
			return false;
		if(!pt->present)
			return false;
		if(!pt->writable) {
			/* we have to handle copy-on-write here manually because the kernel can write
			 * to the page anyway */
			if(pt->copyOnWrite)
				paging_handlePageFault(virtual);
			else
				return false;
		}
		virtual += PAGE_SIZE;
		pt++;
	}
	return true;
}

u32 paging_getFrameOf(u32 virtual) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED(virtual);
	if(pt->present == 0)
		return 0;
	return pt->frameNumber;
}

u32 paging_countFramesForMap(u32 virtual,u32 count) {
	/* we need at least <count> frames */
	u32 res = count;
	/* signed is better here :) */
	s32 c = count;
	sPDEntry *pd;
	paging_mapPageDir();
	while(c > 0) {
		/* page-table not present yet? */
		pd = (sPDEntry*)(PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual) * sizeof(sPDEntry));
		if(!pd->present) {
			res++;
		}

		/* advance to next page-table */
		c -= PT_ENTRY_COUNT;
		virtual += PAGE_SIZE * PT_ENTRY_COUNT;
	}
	return res;
}

void paging_map(u32 virtual,u32 *frames,u32 count,u8 flags,bool force) {
	paging_mapIntern(PAGE_DIR_AREA,MAPPED_PTS_START,virtual,frames,count,flags,force);
}

static void paging_mapIntern(u32 pageDir,u32 mappingArea,u32 virtual,u32 *frames,u32 count,u8 flags,
		bool force) {
	u32 frame;
	sPDEntry *pd;
	sPTEntry *pt;

	vassert(pageDir == PAGE_DIR_AREA || pageDir == PAGE_DIR_TMP_AREA,"pageDir invalid");
	vassert(mappingArea == MAPPED_PTS_START || mappingArea == TMPMAP_PTS_START,"mappingArea invalid");
	vassert(flags & (PG_WRITABLE | PG_SUPERVISOR | PG_COPYONWRITE | PG_ADDR_TO_FRAME),"flags empty");
	vassert(!(flags & ~(PG_WRITABLE | PG_SUPERVISOR | PG_COPYONWRITE | PG_ADDR_TO_FRAME)),
			"flags contain invalid bits");
	vassert(force == true || force == false,"force invalid");

	/* just if we use the default one */
	if(pageDir == PAGE_DIR_AREA)
		paging_mapPageDir();

	virtual &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		pd = (sPDEntry*)pageDir + ADDR_TO_PDINDEX(virtual);
		/* page table not present? */
		if(!pd->present) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				util_panic("Not enough memory");
			}

			pd->ptFrameNo = frame;
			pd->present = true;
			/* TODO always writable? */
			pd->writable = true;
			pd->notSuperVisor = (flags & PG_SUPERVISOR) == 0 ? true : false;

			/* clear frame (ensure that we start at the beginning of the frame) */
			memset((void*)ADDR_TO_MAPPED_CUSTOM(mappingArea,
					virtual & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),0,PAGE_SIZE);

			paging_flushTLB();
		}

		/* setup page */
		pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virtual);
		/* ignore already present entries */
		if(force || !pt->present) {
			if(frames == NULL)
				pt->frameNumber = mm_allocateFrame(MM_DEF);
			else {
				if(flags & PG_ADDR_TO_FRAME)
					pt->frameNumber = *frames++ >> PAGE_SIZE_SHIFT;
				else
					pt->frameNumber = *frames++;
			}
			pt->present = true;
			pt->dirty = false;
			pt->writable = (flags & PG_WRITABLE) ? true : false;
			pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0 ? true : false;
			pt->copyOnWrite = (flags & PG_COPYONWRITE) ? true : false;

			/* invalidate TLB-entry */
			if(pageDir == PAGE_DIR_AREA)
				paging_flushAddr(virtual);
		}

		/* to next page */
		virtual += PAGE_SIZE;
	}
}

void paging_unmap(u32 virtual,u32 count,bool freeFrames,bool remCOW) {
	paging_unmapIntern(proc_getRunning(),MAPPED_PTS_START,virtual,count,freeFrames,remCOW);
}

void paging_unmapPageTables(u32 start,u32 count) {
	paging_unmapPageTablesIntern(PAGE_DIR_AREA,start,count);
}

u32 paging_clonePageDir(u32 *stackFrame,sProc *newProc) {
	bool oldIntrptState;
	u32 x,pdirFrame,frameCount,kheapCount;
	u32 tPages,dPages,sPages;
	sPDEntry *pd,*npd,*tpd;
	sPTEntry *pt;
	sProc *p;

	vassert(stackFrame != NULL,"stackFrame == NULL");
	vassert(newProc != NULL,"newProc == NULL");

	DBG_PGCLONEPD(vid_printf(">>===== paging_clonePageDir(newPid=%d) =====\n",newPid));

	/* note that interrupts have to be disabled, because no other process is allowed to
	 * run during this function since we are calculating the memory we need at the beginning! */
	oldIntrptState = intrpt_setEnabled(false);

	/* frames needed:
	 * 	- page directory
	 * 	- kernel-stack page-table
	 * 	- kernel stack
	 *  - page-tables for text+data
	 *  - page-tables for stack
	 * The frames for the page-content is not yet needed since we're using copy-on-write!
	 */
	p = proc_getRunning();
	tPages = p->textPages;
	dPages = p->dataPages;
	sPages = p->stackPages;
	frameCount = 3 + PAGES_TO_PTS(tPages + dPages) + PAGES_TO_PTS(sPages);
	/* worstcase heap-usage. NOTE THAT THIS ASSUMES A BIT ABOUT THE INTERNAL STRUCTURE OF SLL! */
	kheapCount = (dPages + sPages) * (sizeof(sCOW) + sizeof(sSLNode)) * 2;
	/* TODO finish! */
	if(mm_getFreeFrmCount(MM_DEF) < frameCount/* || kheap_getFreeMem() < kheapCount*/) {
		DBG_PGCLONEPD(vid_printf("Not enough free frames!\n"));
		intrpt_setEnabled(oldIntrptState);
		return 0;
	}

	/* we need a new page-directory */
	pdirFrame = mm_allocateFrame(MM_DEF);
	DBG_PGCLONEPD(vid_printf("Got page-dir-frame %x\n",pdirFrame));

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	paging_map(PAGE_DIR_TMP_AREA,&pdirFrame,1,PG_WRITABLE | PG_SUPERVISOR,true);

	pd = (sPDEntry*)PAGE_DIR_AREA;
	npd = (sPDEntry*)PAGE_DIR_TMP_AREA;

	/* copy old page-dir to new one */
	DBG_PGCLONEPD(vid_printf("Copying old page-dir to new one (%x)\n",npd));
	/* clear user-space page-tables */
	memset(npd,0,ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) * sizeof(sPDEntry));
	/* copy kernel-space page-tables*/
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)) * sizeof(sPDEntry));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	DBG_PGCLONEPD(vid_printf("Create kernel-stack and copy old one into it\n"));
	tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK);
	tpd->ptFrameNo = mm_allocateFrame(MM_DEF);
	tpd->present = true;
	tpd->writable = true;
	/* clear the page-table */
	memset((void*)ADDR_TO_MAPPED_CUSTOM(TMPMAP_PTS_START,
			KERNEL_STACK & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),0,PAGE_SIZE);

	/* now setup the new stack */
	pt = (sPTEntry*)(TMPMAP_PTS_START + (KERNEL_STACK / PT_ENTRY_COUNT));
	pt->frameNumber = mm_allocateFrame(MM_DEF);
	*stackFrame = pt->frameNumber;
	pt->present = true;
	pt->writable = true;

	/* map pages for text to the frames of the old process */
	DBG_PGCLONEPD(vid_printf("Mapping %d text-pages (shared)\n",tPages));
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,
			0,(u32*)MAPPED_PTS_START,tPages,PG_ADDR_TO_FRAME,true);

	/* map pages for data. we will copy the data with copyonwrite. */
	DBG_PGCLONEPD(vid_printf("Mapping %d data-pages (copy-on-write)\n",dPages));
	x = tPages * PAGE_SIZE;
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,
			(u32*)ADDR_TO_MAPPED(x),dPages,PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);
	/* mark as copy-on-write for the current and new process */
	paging_setCOW(x,(u32*)ADDR_TO_MAPPED(x),dPages,newProc);

	/* map pages for stack. we will copy the data with copyonwrite. */
	DBG_PGCLONEPD(vid_printf("Mapping %d stack-pages (copy-on-write)\n",sPages));
	x = KERNEL_AREA_V_ADDR - sPages * PAGE_SIZE;
	paging_mapIntern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,
			(u32*)ADDR_TO_MAPPED(x),sPages,PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);
	/* mark as copy-on-write for the current and new process */
	paging_setCOW(x,(u32*)ADDR_TO_MAPPED(x),sPages,newProc);

	/* unmap stuff */
	DBG_PGCLONEPD(vid_printf("Removing temp mappings\n"));
	paging_unmap(PAGE_DIR_TMP_AREA,1,false,false);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;

	/* one final flush to ensure everything is correct */
	paging_flushTLB();

	/* we're done */
	intrpt_setEnabled(oldIntrptState);

	DBG_PGCLONEPD(vid_printf("<<===== paging_clonePageDir() =====\n"));

	return pdirFrame;
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
		paging_map(KERNEL_STACK_TMP,&frameNumber,1,PG_WRITABLE | PG_SUPERVISOR,true);

		/* copy content */
		memcpy((void*)(address & ~(PAGE_SIZE - 1)),(void*)KERNEL_STACK_TMP,PAGE_SIZE);

		/* unmap */
		paging_unmap(KERNEL_STACK_TMP,1,false,false);
	}

	return true;
}

void paging_destroyPageDir(sProc *p) {
	sPDEntry *pd,*ppd;

	vassert(p != NULL,"p == NULL");

	/* map page-dir of process */
	paging_map(PAGE_DIR_TMP_AREA,&(p->physPDirAddr),1,
			PG_SUPERVISOR | PG_WRITABLE | PG_ADDR_TO_FRAME,true);

	/* map page-tables */
	pd = (sPDEntry*)PAGE_DIR_AREA;
	ppd = (sPDEntry*)PAGE_DIR_TMP_AREA;
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = ppd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	/* we want to access the whole page-table */
	paging_flushTLB();

	/* TODO text should be shared, right? */

	/* free data-pages and page-tables */
	paging_unmapIntern(p,TMPMAP_PTS_START,p->textPages * PAGE_SIZE,p->dataPages,true,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,PAGES_TO_PTS(p->textPages),
			PAGES_TO_PTS(p->textPages + p->dataPages) - PAGES_TO_PTS(p->textPages));

	/* free stack-pages */
	paging_unmapIntern(p,TMPMAP_PTS_START,KERNEL_AREA_V_ADDR - p->stackPages * PAGE_SIZE,
			p->stackPages,true,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,
			ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) - PAGES_TO_PTS(p->stackPages),
			PAGES_TO_PTS(p->stackPages));

	/* free kernel-stack */
	paging_unmapIntern(p,TMPMAP_PTS_START,KERNEL_STACK,1,true,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,ADDR_TO_PDINDEX(KERNEL_STACK),1);

	/* unmap stuff & free page-dir */
	paging_unmap(PAGE_DIR_TMP_AREA,1,true,false);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;

	paging_flushTLB();
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = false;
	paging_flushTLB();
}

static void paging_unmapIntern(sProc *p,u32 mappingArea,u32 virtual,u32 count,bool freeFrames,
		bool remCOW) {
	sPTEntry *pt = (sPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virtual);

	vassert(mappingArea == MAPPED_PTS_START || mappingArea == TMPMAP_PTS_START,"mappingArea invalid");
	vassert(freeFrames == true || freeFrames == false,"freeFrames invalid");

	while(count-- > 0) {
		/* remove and free, if desired */
		if(pt->present) {
			/* we don't want to free copy-on-write pages and not frames in front of the kernel
			 * because they may be mapped more than once and will never be free'd */
			if(freeFrames && pt->frameNumber * PAGE_SIZE >= KERNEL_P_ADDR) {
				if(pt->copyOnWrite && remCOW)
					paging_remFromCow(p,pt->frameNumber);
				else if(!pt->copyOnWrite)
					mm_freeFrame(pt->frameNumber,MM_DEF);
			}
			pt->present = false;

			/* invalidate TLB-entry */
			if(mappingArea == MAPPED_PTS_START)
				paging_flushAddr(virtual);
		}

		/* to next page */
		pt++;
		virtual += PAGE_SIZE;
	}
}

static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count) {
	sPDEntry *pde;

	/* just if we use the default one */
	if(pageDir == PAGE_DIR_AREA)
		paging_mapPageDir();

	pde = (sPDEntry*)pageDir + start;

	vassert(pageDir == PAGE_DIR_AREA || pageDir == PAGE_DIR_TMP_AREA,"pageDir invalid");
	vassert(start < PT_ENTRY_COUNT,"start >= PT_ENTRY_COUNT");
	vassert(count < PT_ENTRY_COUNT,"count >= PT_ENTRY_COUNT");

	while(count-- > 0) {
		pde->present = 0;
		mm_freeFrame(pde->ptFrameNo,MM_DEF);
		pde++;
	}

	if(pageDir == PAGE_DIR_AREA)
		paging_flushTLB();
}

static void paging_setCOW(u32 virtual,u32 *frames,u32 count,sProc *newProc) {
	sPTEntry *ownPt;
	sCOW *cow;

	virtual &= ~(PAGE_SIZE - 1);
	while(count-- > 0) {
		/* build cow-entry for child */
		cow = (sCOW*)kheap_alloc(sizeof(sCOW));
		if(cow == NULL)
			util_panic("Not enough mem for copy-on-write!");
		cow->frameNumber = *frames >> PAGE_SIZE_SHIFT;
		cow->proc = newProc;
		if(!sll_append(cowFrames,cow))
			util_panic("Not enough mem for copy-on-write!");

		/* build cow-entry for parent if not already done */
		ownPt = (sPTEntry*)ADDR_TO_MAPPED(virtual);
		if(!ownPt->copyOnWrite) {
			cow = (sCOW*)kheap_alloc(sizeof(sCOW));
			if(cow == NULL)
				util_panic("Not enough mem for copy-on-write!");
			cow->frameNumber = *frames >> PAGE_SIZE_SHIFT;
			cow->proc = proc_getRunning();
			if(!sll_append(cowFrames,cow))
				util_panic("Not enough mem for copy-on-write!");

			ownPt->copyOnWrite = true;
			ownPt->writable = false;
			ownPt->dirty = false;
			paging_flushAddr(virtual);
		}

		frames++;
		virtual += PAGE_SIZE;
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

static void paging_dbg_printPageDir(u32 mappingArea,u32 pageDirArea,u8 parts);
static void paging_dbg_printPageTable(u32 mappingArea,u32 no,sPDEntry *pde);
static void paging_dbg_printPage(sPTEntry *page);

void paging_dbg_printCOW(void) {
	sSLNode *n;
	sCOW *cow;
	vid_printf("COW-Frames:\n");
	for(n = sll_begin(cowFrames); n != NULL; n = n->next) {
		cow = (sCOW*)n->data;
		vid_printf("\tframe=0x%x, pid=0x%x, pcmd=%s\n",cow->frameNumber,cow->proc->pid,
				cow->proc->command);
	}
}

sPTEntry *paging_dbg_getPTEntry(u32 virtual) {
	paging_mapPageDir();
	return (sPTEntry*)ADDR_TO_MAPPED(virtual);
}

u32 paging_dbg_getPageCount(void) {
	u32 i,x,count = 0;
	sPTEntry *pagetable;
	sPDEntry *pdir = (sPDEntry*)PAGE_DIR_AREA;
	paging_mapPageDir();
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
	sPDEntry *pd,*ppd;

	/* map page-dir */
	paging_map(PAGE_DIR_TMP_AREA,&(p->physPDirAddr),1,
			PG_SUPERVISOR | PG_WRITABLE | PG_ADDR_TO_FRAME,true);
	/* map page-tables */
	pd = (sPDEntry*)PAGE_DIR_AREA;
	ppd = (sPDEntry*)PAGE_DIR_TMP_AREA;
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = ppd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	paging_flushTLB();

	paging_dbg_printPageDir(TMPMAP_PTS_START,PAGE_DIR_TMP_AREA,parts);

	/* unmap */
	paging_unmap(PAGE_DIR_TMP_AREA,1,false,false);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;
	paging_flushTLB();
}

static void paging_dbg_printPageDir(u32 mappingArea,u32 pageDirArea,u8 parts) {
	u32 i;
	sPDEntry *pagedir;
	paging_mapPageDir();
	pagedir = (sPDEntry*)pageDirArea;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pagedir[i].present) {
			if(parts == PD_PART_ALL ||
				(i < ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) && (parts & PD_PART_USER)) ||
				(i >= ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) && i < ADDR_TO_PDINDEX(KERNEL_HEAP_START)
						&& (parts & PD_PART_KERNEL)) ||
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
		vid_printf("raw=0x%08x, frame=0x%x [%c%c%c%c]",*(u32*)page,
				page->frameNumber,page->notSuperVisor ? 'u' : 'k',page->writable ? 'w' : 'r',
				page->global ? 'g' : '-',page->copyOnWrite ? 'c' : '-');
	}
	else {
		vid_printf("-");
	}
}

#endif
