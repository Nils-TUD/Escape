/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/paging.h"
#include "../h/util.h"

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000) */
#define virt2phys(addr) ((u32)addr + 0x40000000)

/* the page-directory for process 0 */
tPDEntry proc0PD[PAGE_SIZE / sizeof(tPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for process 0 */
tPTEntry proc0PT[PAGE_SIZE / sizeof(tPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for the mapped page-tables area */
/* residents in kernel because otherwise we would have to init mm first, and after that init
 * paging again. */
tPTEntry mapPT[PAGE_SIZE / sizeof(tPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

/**
 * Assembler routine to enable paging
 * 
 * @param pageDir the pointer to the page-directory
 */
extern void paging_enable(tPDEntry *pageDir);

/**
 * Assembler routine to flush the TLB
 */
extern void tlb_flush(void);

/**
 * Maps the page-table for the given virtual address to <frame> in the mapped page-tables area.
 * 
 * @param pdir the page-directory
 * @param virtual the virtual address
 * @param frame the frame-number
 */
static void paging_mapPageTable(tPDEntry *pdir,u32 virtual,u32 frame,bool flush) {
	u32 addr = ADDR_TO_MAPPED(virtual);
	tPTEntry* pt = (tPTEntry*)(mapPT + ADDR_TO_PTINDEX(addr));
	pt->physAddress = frame;
	pt->present = 1;
	pt->writable = 1;
	/* TODO necessary? */
	if(flush) {
		tlb_flush();
	}
}

/**
 * Counts the number of present pages in the given page-table
 * 
 * @param pt the page-table
 * @return the number of present pages
 */
static u32 paging_getPTEntryCount(tPTEntry* pt) {
	u32 i,count = 0;
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pt->present) {
			count++;
		}
		pt++;
	}
	return count;
}

void paging_init(void) {
	tPDEntry *pd,*pde;
	tPTEntry *pt,*mpt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */
	
	pd = (tPDEntry*)virt2phys(proc0PD);
	pt = (tPTEntry*)virt2phys(proc0PT);
	mpt = (tPTEntry*)virt2phys(mapPT);

	/* map the first 4MiB at 0xC0000000 */
	addr = KERNEL_AREA_P_ADDR;
	end = KERNEL_AREA_P_ADDR + PT_ENTRY_COUNT * PAGE_SIZE;
	for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
		/* build page-table entry */
		proc0PT[i].physAddress = (u32)addr >> PAGE_SIZE_SHIFT;
		proc0PT[i].present = 1;
		proc0PT[i].writable = 1;
	}

	/* insert page-table in the page-directory */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	pde->ptAddress = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	pde = (tPDEntry*)proc0PD;
	pde->ptAddress = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* clear map page-table */
	memset(mpt,0,PT_ENTRY_COUNT);
	
	/* build pd-entry for map pt */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptAddress = (u32)mpt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* map kernel and ourself :) */
	paging_mapPageTable(pde,KERNEL_AREA_V_ADDR,(u32)pt >> PAGE_SIZE_SHIFT,false);
	paging_mapPageTable(pde,MAPPED_PTS_START,(u32)mpt >> PAGE_SIZE_SHIFT,false);
	
	/* now enable paging */
	paging_enable(pd);
}

void paging_map(tPDEntry *pdir,u32 virtual,u32 *frames,u32 count,u8 flags) {
	u32 frame;
	tPDEntry *pd;
	tPTEntry *pt;
	while(count-- > 0) {
		pd = (tPDEntry*)(pdir + ADDR_TO_PDINDEX(virtual));
		/* page table not present? */
		if(!pd->present) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				panic("Not enough memory");
			}
			
			pd->ptAddress = frame;
			pd->present = 1;
			/* TODO always writable? */
			pd->writable = 1;
			
			/* map the page-table because we need to write to it */
			paging_mapPageTable(pdir,virtual,frame,true);
			
			/* clear frame (ensure that we start at the beginning of the frame) */
			memset((void*)ADDR_TO_MAPPED(virtual & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),0,PT_ENTRY_COUNT);
		}
		
		/* setup page */
		pt = (tPTEntry*)ADDR_TO_MAPPED(virtual);
		pt->physAddress = *frames;
		pt->present = 1;
		pt->writable = flags & PG_WRITABLE;
		pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0;
		
		/* to next page */
		virtual += PAGE_SIZE;
		frames++;
	}
	
	/* TODO necessary? */
	tlb_flush();
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = 0;
	/* TODO necessary? */
	tlb_flush();
}

void paging_unmap(tPDEntry *pdir,u32 virtual,u32 count) {
	while(count-- > 0) {
		
	}
}
