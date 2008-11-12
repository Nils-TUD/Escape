/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/paging.h"
#include "../h/mm.h"
#include "../h/util.h"
#include "../h/video.h"
#include "../h/intrpt.h"

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
	procs[pi].physPDirAddr = (u32)paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
}

u16 proc_getFreePid(void) {
	u16 pid;
	for(pid = 1; pid < PROC_COUNT; pid++) {
		if(procs[pid].pid == 0)
			return pid;
	}
	return 0;
}

bool proc_clone(u16 newPid) {
	u32 x,pdirFrame,frameCount;
	tPDEntry *pd,*npd,*tpd;
	tPTEntry *pt;
	u16 oldPid = pi;
	tProc *p = procs + newPid;

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
	frameCount = 3 + PAGES_TO_PTS(procs[pi].textPages + procs[pi].dataPages) +
		PAGES_TO_PTS(procs[pi].stackPages);
	if(mm_getNumberOfFreeFrames(MM_DEF) < frameCount) {
		DBG_PROC_CLONE(vid_printf("Not enough free frames!\n"));
		return false;
	}

	/* we need a new page-directory */
	pdirFrame = mm_allocateFrame(MM_DEF);
	DBG_PROC_CLONE(vid_printf("Got page-dir-frame %x\n",pdirFrame));

	/* set page-dir and pages for segments */
	p->textPages = procs[pi].textPages;
	p->dataPages = procs[pi].dataPages;
	p->stackPages = procs[pi].stackPages;
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;

	/* Map page-dir into temporary area, so we can access both page-dirs atm */
	paging_map(PAGE_DIR_TMP_AREA,&pdirFrame,1,PG_WRITABLE | PG_SUPERVISOR,true);
	/* we have to write to the temp-area */
	/* TODO optimize! */
	paging_flushTLB();

	pd = (tPDEntry*)PAGE_DIR_AREA;
	npd = (tPDEntry*)PAGE_DIR_TMP_AREA;

	/* copy old page-dir to new one */
	DBG_PROC_CLONE(vid_printf("Copying old page-dir to new one (%x)\n",npd));
	/* clear user-space page-tables */
	memset(npd,0,ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	/* copy kernel-space page-tables*/
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)) * sizeof(u32));

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* make the page-tables of the old process accessible in a different page-table */
	npd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = pd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area and the stack itself */
	/* Note that we have to do this here because as soon as we exchange the page-dir
	 * we will use the new stack. So it has to be present before! */
	tpd = npd + ADDR_TO_PDINDEX(KERNEL_STACK);
	tpd->ptFrameNo = mm_allocateFrame(MM_DEF);
	tpd->present = true;
	tpd->writable = true;
	/* map the page-tables of the new process so that we can access them */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];
	/* TODO optimize! */
	paging_flushTLB();
	/* now setup the new stack */
	pt = (tPTEntry*)(TMPMAP_PTS_START + (KERNEL_STACK / PT_ENTRY_COUNT));
	pt->frameNumber = mm_allocateFrame(MM_DEF);
	pt->present = true;
	pt->writable = true;
	/* unmap page-tables of the new process */
	pd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)].present = false;

	/* exchange page-dir */
	DBG_PROC_CLONE(vid_printf("Exchange page-dir to the new one\n"));
	paging_exchangePDir(pdirFrame << PAGE_SIZE_SHIFT);
	/* we have to exchange the pid for paging_map() and so on */
	pi = newPid;

	/* map pages for text to the frames of the old process */
	DBG_PROC_CLONE(vid_printf("Mapping %d text-pages (shared)\n",p->textPages));
	paging_map(0,(u32*)TMPMAP_PTS_START,p->textPages,PG_ADDR_TO_FRAME,true);

	/* map pages for data. we will copy the data with copyonwrite. */
	DBG_PROC_CLONE(vid_printf("Mapping %d data-pages (copy-on-write)\n",p->dataPages));
	x = 0 + p->textPages * PAGE_SIZE;
	paging_map(x,NULL,p->stackPages,PG_WRITABLE,true);
	/* TODO use copy-on-write */
	/*paging_map(x,(u32*)(TMPMAP_PTS_START + (x / PT_ENTRY_COUNT)),p->dataPages,
			PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);*/

	/* map pages for stack. we will copy the data with copyonwrite. */
	DBG_PROC_CLONE(vid_printf("Mapping %d stack-pages (copy-on-write)\n",p->stackPages));
	x = KERNEL_AREA_V_ADDR - p->stackPages * PAGE_SIZE;
	paging_map(x,NULL,p->stackPages,PG_WRITABLE,true);
	/* TODO use copy-on-write */
	/*paging_map(x,(u32*)(TMPMAP_PTS_START + (x / PT_ENTRY_COUNT)),p->stackPages,
			PG_COPYONWRITE | PG_ADDR_TO_FRAME,true);*/

	/* change back to the original page-dir */
	DBG_PROC_CLONE(vid_printf("Exchanging page-dir back\n"));
	pi = oldPid;
	paging_exchangePDir(procs[pi].physPDirAddr);

	/* remove the temp page-dir */
	DBG_PROC_CLONE(vid_printf("Removing temp page-dir\n"));
	paging_unmap(PAGE_DIR_TMP_AREA,1,false);
	/* TODO optimize! */
	paging_flushTLB();

	intrpt_enable();

	DBG_PROC_CLONE(vid_printf("Done :)\n"));

	return true;
}

bool proc_segSizesValid(u32 textPages,u32 dataPages,u32 stackPages) {
	u32 maxPages = KERNEL_AREA_V_ADDR / PAGE_SIZE;
	return textPages + dataPages + stackPages <= maxPages;
}

bool proc_changeSize(s32 change,chgArea area) {
	u32 addr,chg = change;
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

		paging_map(addr,NULL,change,PG_WRITABLE,false);

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

		/* free and unmap memory */
		paging_unmap(addr,-change,true);

		/* ensure that the TLB contains no invalid entries */
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
