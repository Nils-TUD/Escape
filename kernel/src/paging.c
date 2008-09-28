/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/paging.h"
#include "../h/util.h"
#include "../h/video.h"
#include "../h/proc.h"

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
/* the page-table for the page-dir area */
tPTEntry pdirPT[PAGE_SIZE / sizeof(tPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

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
 * Maps the page-table for the given virtual address to <frame> in the mapped
 * page-tables area.
 * 
 * @param virtual the virtual address
 * @param frame the frame-number
 * @param flush flush the TLB?
 */
static void paging_mapPageTable(u32 virtual,u32 frame,bool flush) {
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
 * Unmaps the page-table for the given virtual address to <frame> out of the mapped
 * page-tables area.
 * 
 * @param virtual the virtual address
 * @param flush flush the TLB?
 */
static void paging_unmapPageTable(u32 virtual,bool flush) {
	u32 addr = ADDR_TO_MAPPED(virtual);
	tPTEntry* pt = (tPTEntry*)(mapPT + ADDR_TO_PTINDEX(addr));
	pt->present = 0;
	
	/* TODO necessary? */
	if(flush) {
		tlb_flush();
	}
}

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

void paging_init(void) {
	tPDEntry *pd,*pde;
	tPTEntry *pt,*mpt,*pdpt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */
	
	pd = (tPDEntry*)virt2phys(proc0PD);
	pt = (tPTEntry*)virt2phys(proc0PT);
	mpt = (tPTEntry*)virt2phys(mapPT);
	pdpt = (tPTEntry*)virt2phys(pdirPT);

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
	
	/* clear pdir page-table */
	memset(pdpt,0,PT_ENTRY_COUNT);
	
	/* build pd-entry for pdir pt */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(PAGE_DIR_AREA));
	pde->ptAddress = (u32)pdpt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* insert the page into the page-table for the page-directory area */
	i = ADDR_TO_PTINDEX(PAGE_DIR_AREA);
	pdirPT[i].physAddress = (u32)pd >> PAGE_SIZE_SHIFT;
	pdirPT[i].present = 1;
	pdirPT[i].writable = 1;
	
	/* clear map page-table */
	memset(mpt,0,PT_ENTRY_COUNT);
	
	/* build pd-entry for map pt */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptAddress = (u32)mpt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;
	
	/* map kernel, page-dir and ourself :) */
	paging_mapPageTable(KERNEL_AREA_V_ADDR,(u32)pt >> PAGE_SIZE_SHIFT,false);
	paging_mapPageTable(PAGE_DIR_AREA,(u32)pdpt >> PAGE_SIZE_SHIFT,false);
	paging_mapPageTable(MAPPED_PTS_START,(u32)mpt >> PAGE_SIZE_SHIFT,false);
	
	/* now enable paging */
	paging_enable(pd);
}

u32 paging_getPageCount(void) {
	u32 i,x,count = 0;
	tPTEntry *pagetable;
	tPDEntry *pdir = (tPDEntry*)PAGE_DIR_AREA;
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

void paging_map(u32 virtual,u32 *frames,u32 count,u8 flags) {
	u32 frame;
	tPDEntry *pd;
	tPTEntry *pt;
	while(count-- > 0) {
		pd = (tPDEntry*)(PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual) * sizeof(tPDEntry));
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
			paging_mapPageTable(virtual,frame,true);
			
			/* clear frame (ensure that we start at the beginning of the frame) */
			memset((void*)ADDR_TO_MAPPED(virtual & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),
					0,PT_ENTRY_COUNT);
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
}

void paging_unmap(u32 virtual,u32 count) {
	u32 pdIndex;
	bool ptEmpty;
	u32 index = ADDR_TO_PTINDEX(virtual);
	tPDEntry *pdir;
	tPTEntry *addr = (tPTEntry*)ADDR_TO_MAPPED(virtual);
	while(count-- > 0) {
		addr->present = 0;
		
		/* check if the page-table is empty if we have reached the end of it
		 * or if we're finished now */
		if(count == 0 || (index % PT_ENTRY_COUNT) == PT_ENTRY_COUNT - 1) {
			pdIndex = ADDR_TO_PDINDEX(virtual);
			ptEmpty = paging_isPTEmpty((tPTEntry*)(MAPPED_PTS_START + pdIndex * PAGE_SIZE));
			/* page-table empty? */
			if(ptEmpty) {
				pdir = (tPDEntry*)PAGE_DIR_AREA;
				/* so remove it from page-dir and free the frame for it */
				pdir[pdIndex].present = 0;
				mm_freeFrame(pdir[pdIndex].ptAddress,MM_DEF);
				/* unmap the page-table */
				paging_unmapPageTable(virtual,true);
			}
		}

		/* to next page */
		addr++;
		index = (index + 1) % PT_ENTRY_COUNT;
		virtual += PAGE_SIZE;
	}
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = 0;
	/* TODO necessary? */
	tlb_flush();
}

#ifdef TEST_PAGING

#define TEST_MAX_FRAMES 1024*3
u32 frames[TEST_MAX_FRAMES];

static bool test_paging_cycle(u32 i,u32 addr,u32 count);
static void test_paging_allocate(u32 addr,u32 count);
static void test_paging_access(u32 addr,u32 count);
static void test_paging_free(u32 addr,u32 count);
	
void test_paging(void) {
	u32 i,x,y;
	u32 addr[] = {0x0,0xB0000000,0xA0000000,0x4000,0x1234};
	u32 count[] = {0,1,50,1024,1025,2048,2051};
	
	i = 0;
	for(y = 0; y < sizeof(addr) / sizeof(addr[0]); y++) {
		for(x = 0; x < sizeof(count) / sizeof(count[0]); x++) {
			test_paging_cycle(i++,addr[y],count[x]);
		}
	}
}

static bool test_paging_cycle(u32 i,u32 addr,u32 count) {
	u32 oldFF, newFF, oldPC, newPC;

	vid_printf("TEST %d: addr=0x%x, count=%d\n",i,addr,count);
	
	oldPC = paging_getPageCount();
	oldFF = mm_getNumberOfFreeFrames();
	
	test_paging_allocate(addr,count);
	test_paging_access(addr,count);
	test_paging_free(addr,count);

	newPC = paging_getPageCount();
	newFF = mm_getNumberOfFreeFrames();
	
	if(oldFF != newFF || oldPC != newPC) {
		vid_printf("FAILED: oldPC=%d, oldFF=%d, newPC=%d, newFF=%d\n\n",oldPC,oldFF,newPC,newFF);
		return false;
	}
	
	vid_printf("SUCCESS!\n\n");
	
	return true;
}

static void test_paging_allocate(u32 addr,u32 count) {
	mm_allocateFrames(MM_DEF,frames,count);
	paging_map(addr,frames,count,PG_WRITABLE);
	
	tlb_flush();
}

static void test_paging_access(u32 addr,u32 count) {
	u32 i;
	/* make page-aligned */
	addr &= ~(PAGE_SIZE - 1);
	for(i = 0; i < count; i++) {
		/* write to the first word */
		*(u32*)addr = 0xDEADBEEF;
		/* write to the last word */
		*(u32*)(addr + PAGE_SIZE - sizeof(u32)) = 0xDEADBEEF;
		addr += PAGE_SIZE;
	}
}

static void test_paging_free(u32 addr,u32 count) {
	paging_unmap(addr,count);
	mm_freeFrames(MM_DEF,frames,count);

	tlb_flush();
}

#endif
