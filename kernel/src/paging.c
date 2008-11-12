/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/paging.h"
#include "../h/util.h"
#include "../h/video.h"
#include "../h/proc.h"

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + ((u32)(addr) / PT_ENTRY_COUNT))

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

void paging_init(void) {
	tPDEntry *pd,*pde;
	tPTEntry *pt;
	u32 i,addr,end;
	/* note that we assume here that the kernel is not larger than a complete page-table (4MiB)! */

	pd = (tPDEntry*)VIRT2PHYS(proc0PD);
	pt = (tPTEntry*)VIRT2PHYS(proc0PT);

	/* map the first 4MiB at 0xC0000000 */
	addr = KERNEL_AREA_P_ADDR;
	end = KERNEL_AREA_P_ADDR + PT_ENTRY_COUNT * PAGE_SIZE;
	for(i = 0; addr < end; i++, addr += PAGE_SIZE) {
		/* build page-table entry */
		proc0PT[i].frameNumber = (u32)addr >> PAGE_SIZE_SHIFT;
		proc0PT[i].present = 1;
		proc0PT[i].writable = 1;
	}

	/* insert page-table in the page-directory */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR));
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;

	/* map it at 0x0, too, because we need it until we've "corrected" our GDT */
	pde = (tPDEntry*)proc0PD;
	pde->ptFrameNo = (u32)pt >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;

	/* insert page-dir into page-table so that we can access our page-dir */
	i = ADDR_TO_PTINDEX(PAGE_DIR_AREA);
	proc0PT[i].frameNumber = (u32)pd >> PAGE_SIZE_SHIFT;

	/* put the page-directory in the last page-dir-slot */
	pde = (tPDEntry*)(proc0PD + ADDR_TO_PDINDEX(MAPPED_PTS_START));
	pde->ptFrameNo = (u32)pd >> PAGE_SIZE_SHIFT;
	pde->present = 1;
	pde->writable = 1;

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
		pde->present = 1;
		pde->writable = 1;
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
	u32 frame;
	tPDEntry *pd;
	tPTEntry *pt;
	paging_mapPageDir();
	while(count-- > 0) {
		pd = (tPDEntry*)PAGE_DIR_AREA + ADDR_TO_PDINDEX(virtual);
		/* page table not present? */
		if(!pd->present) {
			/* get new frame for page-table */
			frame = mm_allocateFrame(MM_DEF);
			if(frame == 0) {
				panic("Not enough memory");
			}

			pd->ptFrameNo = frame;
			pd->present = 1;
			/* TODO always writable? */
			pd->writable = 1;

			/* clear frame (ensure that we start at the beginning of the frame) */
			memset((void*)ADDR_TO_MAPPED(virtual & ~((PT_ENTRY_COUNT - 1) * PAGE_SIZE)),
					0,PT_ENTRY_COUNT);
		}

		/* setup page */
		pt = (tPTEntry*)ADDR_TO_MAPPED(virtual);
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
			pt->present = 1;
			pt->writable = flags & PG_WRITABLE;
			pt->notSuperVisor = (flags & PG_SUPERVISOR) == 0;
			pt->copyOnWrite = flags & PG_COPYONWRITE;
		}

		/* to next page */
		virtual += PAGE_SIZE;
	}
}

void paging_unmap(u32 virtual,u32 count,bool freeFrames) {
	bool ptEmpty,freePT;
	u32 pdIndex,index = ADDR_TO_PTINDEX(virtual);
	tPDEntry *pdir;
	tPTEntry *pt = (tPTEntry*)ADDR_TO_MAPPED(virtual);

	/* don't free page-tables in the kernel-area */
	freePT = virtual < KERNEL_AREA_V_ADDR;
	if(freePT)
		paging_mapPageDir();

	while(count-- > 0) {
		/* remove and free, if desired */
		if(pt->present) {
			if(freeFrames)
				mm_freeFrame(pt->frameNumber,MM_DEF);
			pt->present = false;
		}

		/* check if the page-table is empty if we have reached the end of it
		 * or if we're finished now */
		if(freePT) {
			if(count == 0 || (index % PT_ENTRY_COUNT) == PT_ENTRY_COUNT - 1) {
				pdIndex = ADDR_TO_PDINDEX(virtual);
				ptEmpty = paging_isPTEmpty((tPTEntry*)(MAPPED_PTS_START + pdIndex * PAGE_SIZE));
				/* page-table empty? */
				if(ptEmpty) {
					pdir = (tPDEntry*)PAGE_DIR_AREA;
					/* so remove it from page-dir and free the frame for it */
					pdir[pdIndex].present = 0;
					mm_freeFrame(pdir[pdIndex].ptFrameNo,MM_DEF);
				}
			}

			index = (index + 1) % PT_ENTRY_COUNT;
		}

		/* to next page */
		pt++;
		virtual += PAGE_SIZE;
	}
}

void paging_gdtFinished(void) {
	/* we can simply remove the first page-table since it just a "link" to the "real" page-table
	 * for the kernel */
	proc0PD[0].present = 0;
	/* TODO necessary? */
	paging_flushTLB();
}
