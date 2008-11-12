/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/video.h"
#include "../h/multiboot.h"
#include "../h/gdt.h"
#include "../h/mm.h"
#include "../h/util.h"
#include "../h/paging.h"
#include "../h/proc.h"
#include "../h/intrpt.h"
#include "../h/debug.h"
#include "../h/cpu.h"
#include "../h/elf.h"
#include "../h/kheap.h"

/*
	0x00000000 - 0x000003FF : Real mode interrupt vector table
	0x00000400 - 0x000004FF : BIOS data area
	0x00000500 - 0x00007BFF : Unused
	0x00007C00 - 0x00007DFF : Floppy boot sector is loaded in here
	0x00007E00 - 0x0009FFFF : Unused
	0x000A0000 - 0x000BFFFF : Video RAM video memory
	0x000B0000 - 0x000B7777 : Monochrome video memory (multiple pages)
	0x000B8000 - 0x000BFFFF : Color video memory (multiple pages)
	0x000C0000 - 0x000C7FFF : Video ROM video BIOS
	0x000C8000 - 0x000EFFFF : BIOS shadow area
	0x000F0000 - 0x000FFFFF : System BIOS
*/

/*
 * Important information:
 * 	- EFLAGS-register:		see intel manual, vol3a, page 66
 * 	- Control-Registers:	see intel manual, vol3a, page 71
 * 	- Segment-descriptors:	see intel manual, vol3a, page 101
 * 	- pt- and pd-entries:	see intel manual, vol3a, page 116
 * 	- pmode exceptions:		see intel manual, vol3a, page 191
 * 	- idt-descriptors:		see intel manual, vol3a, page 202
 */

static u8 task1[] = {
	#include "../../build/user_task1.dump"
};

static u8 task2[] = {
	#include "../../build/user_task2.dump"
};

/*
 * load the given elf program, with all its segments.
 * 2 segments are expected. one for text, and the other for data.
 * return 0 for success, nonzero for fail.
 */
s32 loadElfProg(u8 *code);

u32 entryPoint;
bool procsReady = false;

u32 main(tMultiBoot *mbp,u32 magic) {
	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video */
	vid_init();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s\n","DONE");

	printMultiBootInfo();

	/* mm && kheap */
	dbg_startTimer();
	vid_printf("Initializing memory-management...");
	mm_init();
	paging_mapHigherHalf();
	kheap_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	/* processes */
	dbg_startTimer();
	vid_printf("Initializing process-management...");
	proc_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	/* idt */
	dbg_startTimer();
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	vid_printf("Free frames=%d, pages mapped=%d\n",mm_getNumberOfFreeFrames(MM_DMA | MM_DEF),
			paging_getPageCount());

	/*dbg_printPageDir(true);*/

#if 1
	/* TODO the following is just temporary! */
	/* load task1 */
	loadElfProg(task1);

	/*dbg_printPageDir(false);*/

	/* clone ourself */
	u16 pid = proc_getFreePid();
	proc_clone(pid);
	/* save the state for task2 */
	if(proc_save(&procs[pid].save)) {
		/* now load task2 */
		vid_printf("Loading process %d\n",pid);
		loadElfProg(task2);
		vid_printf("Starting...\n");
		return 0;
	}

	/*dbg_printPageDir(false);*/

	procsReady = true;

	/* FIXME note that this is REALLY dangerous! we have just 1 stack at the moment. That means
	 * if we do anything here that manipulates the stack the process we create above will get
	 * an invalid stack
	 */
#else
	while(1);
#endif

	return 0;
}

s32 loadElfProg(u8 *code) {
	u32 seenLoadSegments = 0;

	u32 j;
	u8 const *datPtr;
	Elf32_Ehdr *eheader = (Elf32_Ehdr*)code;
	Elf32_Phdr *pheader = NULL;

	entryPoint = (u32)eheader->e_entry;

	if(*(u32*)eheader->e_ident != *(u32*)ELFMAG) {
		vid_printf("Error: Invalid magic-number\n");
		return -1;
	}

	procs[pi].textPages = 0;
	procs[pi].dataPages = 0;

	/* load the LOAD segments. */
	for(datPtr = (u8 const*)(code + eheader->e_phoff), j = 0; j < eheader->e_phnum;
		datPtr += eheader->e_phentsize, j++) {
		pheader = (Elf32_Phdr*)datPtr;
		if(pheader->p_type == PT_LOAD) {
			u32 pages;
			u8 const* segmentSrc;
			segmentSrc = code + pheader->p_offset;

			/* get to know the lowest virtual address. must be 0x0.  */
			if(seenLoadSegments == 0) {
				if(pheader->p_vaddr != 0) {
					vid_printf("Error: p_vaddr != 0\n");
					return -1;
				}
			}
			else if(seenLoadSegments == 2) {
				/* uh oh a third LOAD segment. that's not allowed
				* indeed */
				vid_printf("Error: too many LOAD segments\n");
				return -1;
			}

			/* Note that we put everything in the data-segment here because otherwise we would
			* steal the text from the parent-process after fork, exec & exit */
			pages = BYTES_2_PAGES(pheader->p_memsz);
			if(seenLoadSegments != 0) {
				if(pheader->p_vaddr & (0x1000-1))
					pages++;
			}

			/* get more space for the data area. */
			proc_changeSize(pages,CHG_DATA);

			/* copy the data, and zero remaining bytes */
			memcpy((void*)pheader->p_vaddr, (void*)segmentSrc, pheader->p_filesz);
			memset((void*)(pheader->p_vaddr + pheader->p_filesz), 0, pheader->p_memsz - pheader->p_filesz);
			++seenLoadSegments;
		}
	}

	/* give the process 2 stack pages */
	proc_changeSize(2,CHG_STACK);

	return 0;
}
