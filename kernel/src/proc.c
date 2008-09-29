/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/proc.h"
#include "../h/paging.h"
#include "../h/mm.h"
#include "../h/util.h"
#include "../h/video.h"

/* our processes */
tProc procs[PROC_COUNT];
/* the process-index */
u32 pi;

void proc_init(void) {
	/* init the first process */
	pi = 0;
	procs[pi].pid = 0;
	procs[pi].parentPid = 0;
	/* the first process has no text, data and stack */
	procs[pi].textPages = 0;
	procs[pi].dataPages = 0;
	procs[pi].stackPages = 0;
	/* note that this assumes that the page-dir is initialized */
	procs[pi].physPDirAddr = (u32)proc0PD & ~KERNEL_AREA_V_ADDR;
}

bool proc_clone(tProc *p) {
	u32 x,pdirFrame,pdirAreaFrame,mapAreaFrame;
	tPDEntry *pd,*npd,*pdapt;
	tPTEntry *pt;
	
	/* frames needed:
	 * 	- page directory
	 * 	- page directory area page-table
	 * 	- map area page-table
	 * The frames for the page-content is not yet needed since we're using copy-on-write!
	 */
	if(mm_getNumberOfFreeFrames(MM_DEF) < 3) {
		return false;
	}
	
	/* TODO note that interrupts have to be disabled, because no other process is allowed to
	 * run during this function since we are calculating the memory we need at the beginning! */
	
	/* we need a new page-directory */
	pdirFrame = mm_allocateFrame(MM_DEF);
	
	/* Map page-dir into temporary area, so we can access both page-dirs atm.
	 * Note that we assume here that we don't need a new page-table for it (may cause panic).
	 * We can do that because we already have the page-table for the current page-dir which
	 * lies in the same page-table */
	paging_map(PAGE_DIR_TMP_AREA,&pdirFrame,1,PG_WRITABLE | PG_SUPERVISOR);
	/* we have to write to the temp-area */
	/* TODO optimize! */
	paging_flushTLB();
	
	pd = (tPDEntry*)PAGE_DIR_AREA;
	npd = (tPDEntry*)PAGE_DIR_TMP_AREA;
	
	/* clear new page-dir */
	memset(npd,0,PT_ENTRY_COUNT);

	/* copy pd-entry for kernel */
	npd[ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)] = pd[ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)];

	/* we need a new frame for the page-dir-area */
	pdirAreaFrame = mm_allocateFrame(MM_DEF);
	pdapt = (tPDEntry*)(npd + ADDR_TO_PDINDEX(PAGE_DIR_AREA));
	pdapt->ptFrameNo = pdirAreaFrame;
	pdapt->present = 1;
	pdapt->writable = 1;
	
	/* we have to write to the page-table */
	paging_map(PAGE_TABLE_AREA,&pdirAreaFrame,1,PG_WRITABLE | PG_SUPERVISOR);
	/* TODO optimize! */
	paging_flushTLB();
	
	/* clear new page-table */
	memset((void*)PAGE_TABLE_AREA,0,PT_ENTRY_COUNT);
	
	/* create the page-table-entry for the page-dir-area of the new process */
	pt = (tPTEntry*)(PAGE_TABLE_AREA + ADDR_TO_PTINDEX(PAGE_DIR_AREA) * sizeof(tPTEntry));
	pt->frameNumber = pdirFrame;
	pt->present = 1;
	pt->writable = 1;
	
	/* we need a frame for the page-tables map page-table :) */
	mapAreaFrame = mm_allocateFrame(MM_DEF);

	/* we have to write to the page-table */
	paging_map(PAGE_TABLE_AREA,&mapAreaFrame,1,PG_WRITABLE | PG_SUPERVISOR);
	/* TODO optimize! */
	paging_flushTLB();
	
	/* clear new page-table */
	memset((void*)PAGE_TABLE_AREA,0,PT_ENTRY_COUNT);

	/* map kernel, page-dir and ourself :) */
	paging_mapPageTable((tPTEntry*)PAGE_TABLE_AREA,KERNEL_AREA_V_ADDR,
			pd[ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)].ptFrameNo,false);
	paging_mapPageTable((tPTEntry*)PAGE_TABLE_AREA,PAGE_DIR_AREA,pdirAreaFrame,false);
	paging_mapPageTable((tPTEntry*)PAGE_TABLE_AREA,MAPPED_PTS_START,mapAreaFrame,false);
	
	/* insert into page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = mapAreaFrame;
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].present = 1;
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].writable = 1;
	
	/* make the page-tables of the old process accessible in a different page-table */
	npd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = pd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	/* map the page-table for the page-tables of the old-process */
	paging_mapPageTable((tPTEntry*)PAGE_TABLE_AREA,TMPMAP_PTS_START,
			pd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo,false);
	
	/* exchange page-dir */
	paging_exchangePDir(pdirFrame << PAGE_SIZE_SHIFT);
	
	/* map pages for text to the frames of the old process */
	paging_map(0,(u32*)TMPMAP_PTS_START,procs[pi].textPages,0);
	
	/* map pages for data. we will copy the data with copyonwrite. */
	x = 0 + procs[pi].textPages * PAGE_SIZE;
	paging_map(x,(u32*)TMPMAP_PTS_START + ADDR_TO_PTINDEX(x),procs[pi].dataPages,PG_COPYONWRITE);
	
	/* map pages for stack. we will copy the data with copyonwrite. */
	x = KERNEL_AREA_V_ADDR - procs[pi].stackPages * PAGE_SIZE;
	paging_map(x,(u32*)TMPMAP_PTS_START + ADDR_TO_PTINDEX(x),procs[pi].stackPages,PG_COPYONWRITE);

	/* remove the temp-map-area */
	npd = (tPDEntry*)PAGE_DIR_AREA;
	npd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = 0;
	paging_unmapPageTable((tPTEntry*)ADDR_TO_MAPPED(MAPPED_PTS_START),TMPMAP_PTS_START,false);
	
	/*vid_printf("========= NEW PAGE TABLE =========\n");
	dbg_printPageDir();*/
	
	/* set page-dir and pages for segments */
	p->textPages = procs[pi].textPages;
	p->dataPages = procs[pi].dataPages;
	p->stackPages = procs[pi].stackPages;
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	
	/* change back to the original page-dir */
	paging_exchangePDir(procs[pi].physPDirAddr);
	
	/* remove the temp-mappings */
	paging_unmap(PAGE_TABLE_AREA,1);
	paging_unmap(PAGE_DIR_TMP_AREA,1);
	/* TODO optimize! */
	paging_flushTLB();

	/*vid_printf("========= OLD PAGE TABLE =========\n");
	dbg_printPageDir();*/
	
	return true;
}

bool proc_segSizesValid(u32 textPages,u32 dataPages,u32 stackPages) {
	u32 maxPages = KERNEL_AREA_V_ADDR / PAGE_SIZE;
	return textPages + dataPages + stackPages <= maxPages;
}

bool proc_changeSize(s32 change,chgArea area) {
	u32 addr,i,chg = change;
	tPTEntry *pt;
	/* determine start-address */
	if(area == CHG_DATA) {
		addr = (procs[pi].textPages + procs[pi].dataPages) * PAGE_SIZE;
	}
	else {
		addr = KERNEL_AREA_V_ADDR - (procs[pi].stackPages + change) * PAGE_SIZE;
	}
	
	if(change > 0) {
		u32 ts,ds,ss;
		/* not enough mem? */
		if(mm_getNumberOfFreeFrames(MM_DEF) < paging_countFramesForMap(addr,change)) {
			return false;
		}
		/* invalid segment sizes? */
		ts = procs[pi].textPages;
		ds = procs[pi].dataPages;
		ss = procs[pi].stackPages;
		if((area == CHG_DATA && !proc_segSizesValid(ts,ds + change,ss))
				|| (area == CHG_STACK && !proc_segSizesValid(ts,ds,ss + change))) {
			return false;
		}
		
		paging_map(addr,NULL,change,PG_WRITABLE);
		
		/* now clear the memory */
		/* TODO optimize! */
		paging_flushTLB();
		while(change-- > 0) {
			memset((void*)addr,0,PT_ENTRY_COUNT);
			addr += PAGE_SIZE;
		}
	}
	else {
		/* we have to correct the address */
		addr += change * PAGE_SIZE;
		
		/* we have to free the frames for the data first (the page-table may not exist anymore 
		 * after paging_unmap) */
		pt = (tPTEntry*)ADDR_TO_MAPPED(addr);
		i = -change;
		while(i-- > 0) {
			mm_freeFrame(pt->frameNumber,MM_DEF);
			pt++;
		}
		
		/* now unmap the area and - maybe - free page-tables */
		paging_unmap(addr,-change);
		/* finally, ensure that the TLB contains no invalid entries */
		/* TODO optimize! */
		paging_flushTLB();
	}
	
	/* adjust sizes */
	if(area == CHG_DATA) {
		procs[pi].dataPages += chg;
	}
	else {
		procs[pi].stackPages += chg;
	}
	
	return true;
}

#ifdef TEST_PROC

static void test_proc_chgSize(u32 no,s32 change,chgArea area);

void test_proc(void) {
	/*while(1) {
		vid_printf("free frames (MM_DEF) = %d\n",mm_getNumberOfFreeFrames(MM_DEF));
		if(!proc_clone(procs + 1)) {
			panic("proc_clone: Not enough memory!!!");
		}
	}*/
	
	u32 i = 0,x,y;
	s32 changes[] = {0,1,10,1024,1025,2048,2047,2049,mm_getNumberOfFreeFrames(MM_DEF) + 1};
	chgArea areas[] = {CHG_DATA,CHG_STACK};
	
	for(y = 0; y < sizeof(areas) / sizeof(areas[0]); y++) {
		for(x = 0; x < sizeof(changes) / sizeof(changes[0]); x++) {
			test_proc_chgSize(i++,changes[x],areas[y]);
		}
	}
}

static void test_proc_chgSize(u32 no,s32 change,chgArea area) {
	bool res = true;
	u32 oldFF, newFF, oldPC, newPC;
	
	vid_printf("TEST %d (change=%d, area=%d)\n",no,change,area);

	oldPC = paging_getPageCount();
	oldFF = mm_getNumberOfFreeFrames(MM_DEF);
	
	res = res && (change > oldFF ? !proc_changeSize(change,area) : proc_changeSize(change,area));
	if(change <= oldFF) {
		res = res && proc_changeSize(-change,area);
	}
	else {
		vid_printf("Not enough mem or invalid segment sizes!\n");
	}

	newPC = paging_getPageCount();
	newFF = mm_getNumberOfFreeFrames(MM_DEF);
	
	if(!res || oldFF != newFF || oldPC != newPC) {
		vid_printf("FAILED: oldPC=%d, oldFF=%d, newPC=%d, newFF=%d\n\n",oldPC,oldFF,newPC,newFF);
	}
	else {
		vid_printf("SUCCESS!\n");
	}
	
	vid_printf("\n");
}

#endif
