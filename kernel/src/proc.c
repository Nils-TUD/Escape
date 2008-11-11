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

bool proc_clone(tProc *p) {
	u32 x,pdirFrame,stackFrame,stackPTFrame;
	tPDEntry *pd,*npd;

	/* note that interrupts have to be disabled, because no other process is allowed to
	 * run during this function since we are calculating the memory we need at the beginning! */
	intrpt_disable();

	/* frames needed:
	 * 	- page directory
	 * 	- kernel-stack page-table
	 * 	- kernel stack
	 * The frames for the page-content is not yet needed since we're using copy-on-write!
	 * TODO several page-tables are missing here!
	 */
	if(mm_getNumberOfFreeFrames(MM_DEF) < 3) {
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

	dbg_printPageDir(true);

	/* copy old page-dir to new one */
	DBG_PROC_CLONE(vid_printf("Copying old page-dir to new one (%x)\n",npd));
	/* clear user-space page-tables */
	memset(npd,0,ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	/* copy kernel-space page-tables*/
	memcpy(npd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			pd + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR),
			(PT_ENTRY_COUNT - ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR)) * sizeof(u32));

	dbg_printPageDir(true);

	/* map our new page-dir in the last slot of the new page-dir */
	npd[ADDR_TO_PDINDEX(MAPPED_PTS_START)].ptFrameNo = pdirFrame;

	/* make the page-tables of the old process accessible in a different page-table */
	npd[ADDR_TO_PDINDEX(TMPMAP_PTS_START)] = pd[ADDR_TO_PDINDEX(MAPPED_PTS_START)];

	/* get new page-table for the kernel-stack-area */
	/* TODO we have to finish the new stack here!! */
	stackPTFrame = mm_allocateFrame(MM_DEF);
	npd[ADDR_TO_PDINDEX(KERNEL_STACK)].ptFrameNo = stackPTFrame;

	/* exchange page-dir */
	DBG_PROC_CLONE(vid_printf("Exchange page-dir to the new one\n"));
	paging_exchangePDir(pdirFrame << PAGE_SIZE_SHIFT);
	/* TODO we have to exchange the pid for paging_map() and so on */
	pi = 1;

	/* kernel-stack */
	stackFrame = mm_allocateFrame(MM_DEF);
	paging_map(KERNEL_STACK,&stackFrame,1,PG_WRITABLE | PG_SUPERVISOR,true);

	/* map pages for text to the frames of the old process */
	DBG_PROC_CLONE(vid_printf("Mapping text-pages (shared)\n"));
	paging_map(0,(u32*)TMPMAP_PTS_START,procs[pi].textPages,0,true);

	/* map pages for data. we will copy the data with copyonwrite. */
	DBG_PROC_CLONE(vid_printf("Mapping data (copy-on-write)\n"));
	x = 0 + procs[pi].textPages * PAGE_SIZE;
	paging_map(x,(u32*)TMPMAP_PTS_START + ADDR_TO_PTINDEX(x),procs[pi].dataPages,PG_COPYONWRITE,true);

	/* map pages for stack. we will copy the data with copyonwrite. */
	DBG_PROC_CLONE(vid_printf("Mapping stack (copy-on-write)\n"));
	x = KERNEL_AREA_V_ADDR - procs[pi].stackPages * PAGE_SIZE;
	paging_map(x,(u32*)TMPMAP_PTS_START + ADDR_TO_PTINDEX(x),procs[pi].stackPages,PG_COPYONWRITE,true);

	/* change back to the original page-dir */
	DBG_PROC_CLONE(vid_printf("Exchanging page-dir back\n"));
	paging_exchangePDir(procs[pi].physPDirAddr);
	/* TODO exchange pid back */
	pi = 0;

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
