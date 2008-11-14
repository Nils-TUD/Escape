/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/paging.h"
#include "../h/util.h"
#include "../h/video.h"
#include "../h/proc.h"
#include "../h/intrpt.h"

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + ((u32)(addr) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + ((u32)(addr) / PT_ENTRY_COUNT))

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000) */
#define VIRT2PHYS(addr) ((u32)(addr) + 0x40000000)

/* the page-directory for process 0 */
tPDEntry proc0PD[PAGE_SIZE / sizeof(tPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for process 0 */
tPTEntry proc0PT[PAGE_SIZE / sizeof(tPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

/**
 * Assembler routine to enable paging
 */
extern void paging_enable(void);

/**
 * Checks wether the given page-table is empty
 *
 * @param pt the pointer to the first entry of the page-table
 * @return true if empty
 */
static bool paging_isPTEmpty(tPTEntry *pt) {
	u32 i;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pt->present) {
			return false;
		}
		pt++;
	}
	return true;
}

/**
 * Counts the number of present pages in the given page-table
 *
 * @param pt the page-table
 * @return the number of present pages
 */
static u32 paging_getPTEntryCount(tPTEntry *pt) {
	u32 i,count = 0;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pt->present) {
			count++;
		}
		pt++;
	}
	return count;
}

/**
 * paging_map() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 * @param mappingArea the address of the mapping area to use
 */
static void paging_mapintern(u32 pageDir,u32 mappingArea,u32 virtual,u32 *frames,u32 count,u8 flags,
		bool force);

/**
 * paging_unmap() for internal usages
 *
 * @param mappingArea the address of the mapping area to use
 */
static void paging_unmapIntern(u32 mappingArea,u32 virtual,u32 count,bool freeFrames);

/**
 * paging_unmapPageTables() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 */
static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count);

void paging_init(void) {
	tPDEntry *pd,*pde;
	tPTEntry *pt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */

	pd = (tPDEntry*)VIRT2PHYS(proc0PD);
	pt = (tPTEntry*)VIRT2PHYS(proc0PT);

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
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	pde = (tPDEntry*)proc0PD;
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* insert page-dir into page-table so that we can access our page-dir */
	pt = (tPTEntry*)(proc0PT + ADDR_TO_PTINDEX(PAGE_DIR_AREA));
	pt->frameNumber = (u32)pd >> PAGE_SIZE_SHIFT;
	pt->present = true;
	pt->writable = true;

	/* put the page-directory in the last page-dir-slot */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptFrameNo = (u32)pd >> PAGE_SIZE_SHIFT;
	pde->present = true;
	pde->writable = true;

	/* now set page-dir and enable paging */
	paging_exchangePDir((u32)pd);
	paging_enable();
}

void paging_mapHigherHalf(void) {
	u32 addr,end;
	tPDEntry *pde;
	/* insert all page-tables for 0xC0400000 .. 0xFF3FFFFF into the page dir */
	addr = KERNEL_AREA_V_ADDR + (PAGE_SIZE * PT_ENTRY_COUNT);
	end = TMPMAP_PTS_START;
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(addr));
	while(addr < end) {
		/* get frame and insert into page-dir */
		u32 frame = mm_allocateFrame(MM_DEF);
		pde->ptFrameNo = frame;
		pde->present = true;
		pde->writable = true;
		/* clear */
		memset((void*)ADDR_TO_MAPPED(addr),0,PT_ENTRY_COUNT);
		/* to next */
		pde++;
		addr += PAGE_SIZE * PT_ENTRY_COUNT;
	}
}

tPDEntry *paging_getProc0PD(void) {
	return proc0PD;
}

void paging_mapPageDir(void) {
	tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED(PAGE_DIR_AREA);
	u32 pdirFrame = (procs[pi].physPDirAddr >> PAGE_SIZE_SHIFT);
	/* not the current one? */
	if(pt->frameNumber != pdirFrame) {
		pt->frameNumber = pdirFrame;
		/* TODO optimize! */
		paging_flushTLB();
	}
}

u32 paging_getPageCount(void) {
	u32 i,x,count = 0;
	tPTEntry *pagetable;
	tPDEntry *pdir = (tPDEntry*)PAGE_DIR_AREA;
	paging_mapPageDir();
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pdir[i].present) {
			pagetable = (tPTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE);
			for(x = 0; x < PT_ENTRY_COUNT; x++) {
				if(pagetable[x].present) {
					count++;
				}
			}
		}
	}
	return count;
}

u32 paging_getFrameOf(u32 virtual) {
	tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED(virtual);
	return pt->frameNumber;
}

u32 paging_countFramesForMap(u32 virtual,u32 count) {
	/* we need at least <count> frames */
	u32 res = count;
	/* signed is better here :) */
	s32 c = count;
	tPDEntry *pd;
	paging_mapPageDir();
	while(c > 0) {
		/* page-table not present yet? */
		pd = (tPDEntry*)(PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual) * sizeof(tPDEntry));
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
	paging_mapintern(PAGE_DIR_AREA,MAPPED_PTS_START,virtual,frames,count,flags,force);
}

static void paging_mapintern(u32 pageDir,u32 mappingArea,u32 virtual,u32 *frames,u32 count,u8 flags,
		bool force) {
	u32 frame;
	tPDEntry *pd;
	tPTEntry *pt;
	/* just if we use the default one */
	if(pageDir == PAGE_DIR_AREA)
		paging_mapPageDir();

	while(count-- > 0) {
		pd = (tPDEntry*)pageDir + ADDR_TO_PDINDEX(virtual);
		/* page table not present? */
		if(!pd->present) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				panic("Not enough memory");
			}

			pd->ptFrameNo = frame;
			pd->present = true;
			/* TODO always writable? */
			pd->writable = true;
			pd->notSuperVisor = (flags & PG_SUPERVISOR) == 0;

			/* clear frame (ensure that we start at the beginning of the frame) */
			memset((void*)ADDR_TO_MAPPED_CUSTOM(mappingArea,
					virtual & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),0,PT_ENTRY_COUNT);
		}

		/* setup page */
		pt = (tPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virtual);
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
			pt->writable = flags & PG_WRITABLE;
			pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0;
			pt->copyOnWrite = flags & PG_COPYONWRITE;
		}

		/* to next page */
		virtual += PAGE_SIZE;
	}
}

void paging_unmap(u32 virtual,u32 count,bool freeFrames) {
	paging_unmapIntern(MAPPED_PTS_START,virtual,count,freeFrames);
}

static void paging_unmapIntern(u32 mappingArea,u32 virtual,u32 count,bool freeFrames) {
	tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED_CUSTOM(mappingArea,virtual);
	while(count-- > 0) {
		/* remove and free, if desired */
		if(pt->present) {
			if(freeFrames)
				mm_freeFrame(pt->frameNumber,MM_DEF);
			pt->present = false;
		}

		/* to next page */
		pt++;
		virtual += PAGE_SIZE;
	}
}

void paging_unmapPageTables(u32 start,u32 count) {
	paging_unmapPageTablesIntern(PAGE_DIR_AREA,start,count);
}

static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count) {
	tPDEntry *pde = (tPDEntry*)pageDir + start;
	while(count-- > 0) {
		pde->present = 0;
		mm_freeFrame(pde->ptFrameNo,MM_DEF);
		pde++;
	}
}

u32 paging_clonePageDir(u16 newPid,u32 *stackFrame) {
	u32 x,pdirFrame,frameCount;
	u32 tPages,dPages,sPages;
	tPDEntry *pd,*npd,*tpd;
	tPTEntry *pt;

	DBG_PGCLONEPD(vid_printf(">>===== paging_clonePageDir(newPid=%d) =====\n",newPid));

	/* note that interrupts have to be disabled, because no other process is allowed to
	 * run during this function since we are calculating the memory we need at the beginning! */
	intrpt_disable();

	/* frames needed:
	 * 	- page directory
	 * 	- kernel-stack page-table
	 * 	- kernel stack
	 *  - page-tables for text+data
	 *  - page-tables for stack
	 * The frames for the page-content is not yet needed since we're using copy-on-write!
	 */
	tPages = procs[pi].textPages;
	dPages = procs[pi].dataPages;
	sPages = procs[pi].stackPages;
	frameCount = 3 + PAGES_TO_PTS(tPages + dPages) + PAGES_TO_PTS(sPages);
	if(mm_getNumberOfFreeFrames(MM_DEF) < frameCount) {
		DBG_PGCLONEPD(vid_printf("Not enough free frames!\n"));
		intrpt_enable();
		return 0;
	}

	/* we need a new page-directory */
	pdirFrame = mm_allocateFrame(MM_DEF);
	/* needed for paging_mapPageDir() */
	procs[newPid].physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	DBG_PGCLONEPD(vid_printf("Got page-dir-frame %x\n",pdirFrame));

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	paging_map(PAGE_DIR_TMP_AREA,&pdirFrame,1,PG_WRITABLE | PG_SUPERVISOR,true);
	/* we have to write to the temp-area */
	/* TODO optimize! */
	paging_flushTLB();

	pd = (tPDEntry*)PAGE_DIR_AREA;
	npd = (tPDEntry*)PAGE_DIR_TMP_AREA;

	/* copy old page-dir to new one */
	DBG_PGCLONEPD(vid_printf("Copying old page-dir to new one (%x)\n",npd));
	/* clear user-space page-tables */
	memset(npd,0,ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	/* copy kernel-space page-tables*/
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)) * sizeof(u32));

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
	/* TODO optimize! */
	paging_flushTLB();
	/* now setup the new stack */
	pt = (tPTEntry*)(TMPMAP_PTS_START + (KERNEL_STACK / PT_ENTRY_COUNT));
	pt->frameNumber = mm_allocateFrame(MM_DEF);
	*stackFrame = pt->frameNumber;
	pt->present = true;
	pt->writable = true;

	/* map pages for text to the frames of the old process */
	DBG_PGCLONEPD(vid_printf("Mapping %d text-pages (shared)\n",tPages));
	paging_mapintern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,
			0,(u32*)MAPPED_PTS_START,tPages,PG_ADDR_TO_FRAME,true);

	/* map pages for data. we will copy the data with copyonwrite. */
	DBG_PGCLONEPD(vid_printf("Mapping %d data-pages (copy-on-write)\n",dPages));
	x = 0 + tPages * PAGE_SIZE;
	paging_mapintern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,NULL,dPages,PG_WRITABLE,true);
	/* TODO use copy-on-write */
	/*paging_map(x,(u32*)(MAPPED_PTS_START + (x / PT_ENTRY_COUNT)),p->dataPages,
			PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);*/

	/* map pages for stack. we will copy the data with copyonwrite. */
	DBG_PGCLONEPD(vid_printf("Mapping %d stack-pages (copy-on-write)\n",sPages));
	x = KERNEL_AREA_V_ADDR - sPages * PAGE_SIZE;
	paging_mapintern(PAGE_DIR_TMP_AREA,TMPMAP_PTS_START,x,NULL,sPages,PG_WRITABLE,true);
	/* TODO use copy-on-write */
	/*paging_map(x,(u32*)(MAPPED_PTS_START + (x / PT_ENTRY_COUNT)),p->stackPages,
			PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);*/

	/* unmap stuff */
	DBG_PGCLONEPD(vid_printf("Removing temp mappings\n"));
	paging_unmap(PAGE_DIR_TMP_AREA,1,false);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;
	/* TODO optimize! */
	paging_flushTLB();

	/* we're done */
	intrpt_enable();

	DBG_PGCLONEPD(vid_printf("<<===== paging_clonePageDir() =====\n"));

	return pdirFrame;
}

void paging_destroyPageDir(tProc *p) {
	tPDEntry *pd,*ppd;

	/* map page-dir of process */
	paging_map(PAGE_DIR_TMP_AREA,&(p->physPDirAddr),1,
			PG_SUPERVISOR | PG_WRITABLE | PG_ADDR_TO_FRAME,true);
	/* TODO optimize */
	paging_flushTLB();

	/* map page-tables */
	pd = (tPDEntry*)PAGE_DIR_AREA;
	ppd = (tPDEntry*)PAGE_DIR_TMP_AREA;
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = ppd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	/* TODO optimize */
	paging_flushTLB();

	/* TODO text should be shared, right? */

	/* free data-pages and page-tables */
	paging_unmapIntern(TMPMAP_PTS_START,p->textPages * PAGE_SIZE,p->dataPages,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,PAGES_TO_PTS(p->textPages),
			PAGES_TO_PTS(p->textPages + p->dataPages) - PAGES_TO_PTS(p->textPages));

	/* free stack-pages */
	paging_unmapIntern(TMPMAP_PTS_START,KERNEL_AREA_V_ADDR - p->stackPages * PAGE_SIZE,
			p->stackPages,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,
			ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR) - PAGES_TO_PTS(p->stackPages),
			PAGES_TO_PTS(p->stackPages));

	/* free kernel-stack */
	paging_unmapIntern(TMPMAP_PTS_START,KERNEL_STACK,1,true);
	paging_unmapPageTablesIntern(PAGE_DIR_TMP_AREA,ADDR_TO_PDINDEX(KERNEL_STACK),1);

	/* unmap stuff & free page-dir */
	paging_unmap(PAGE_DIR_TMP_AREA,1,true);
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = 0;
	/* TODO necessary? */
	paging_flushTLB();
}
