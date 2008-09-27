/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/paging.h"
#include "../h/util.h"

/* converts the given virtual address to a physical */
#define virt2phys(addr) ((u32)addr + 0x40000000)

/* the page-directory for process 0 */
tPDEntry proc0PD[PAGE_SIZE / sizeof(tPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for process 0 */
tPTEntry proc0PT[PAGE_SIZE / sizeof(tPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

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

void paging_init(void) {
	tPDEntry *pd,*pde;
	tPTEntry *pt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */
	
	pd = (tPDEntry*)virt2phys(proc0PD);
	pt = (tPTEntry*)virt2phys(proc0PT);

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
	pde = (tPDEntry*)(proc0PD + KERNEL_AREA_V_ADDR / PAGE_SIZE / PT_ENTRY_COUNT);
	pde->ptAddress = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	pde = (tPDEntry*)proc0PD;
	pde->ptAddress = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* now enable paging */
	paging_enable(pd);
}

void paging_map(tPDEntry *pdir,s8 *virtual,u32 *frames,u32 count,u8 flags) {
	u32 frame;
	tPDEntry *pd;
	tPTEntry *pt;
	while(count-- > 0) {
		pd = (tPDEntry*)(pdir + (u32)virtual / PAGE_SIZE / PT_ENTRY_COUNT);
		/* page table not present? */
		if(pd->present == 0) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				panic("Not enough memory");
			}
			
			pd->ptAddress = frame;
			pd->present = 1;
			/* TODO always writable? */
			pd->writable = 1;
		}
		
		/* setup page */
		pt = (tPTEntry*)(((u32)pd->ptAddress << PAGE_SIZE_SHIFT) | KERNEL_AREA_V_ADDR);
		pt->physAddress = *frames;
		pt->present = 1;
		pt->writable = flags & PG_WRITABLE;
		pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0;
		
		/* to next page */
		virtual += PAGE_SIZE;
		frames++;
	}
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = 0;
	/* TODO necessary? */
	tlb_flush();
}

void paging_unmap(tPDEntry *pdir,s8 *virtual,u32 count) {
	/* TODO */
}
