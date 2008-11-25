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
#include "../h/elf.h"
#include "../h/sched.h"
#include "../h/vfs.h"

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

s32 main(sMultiBoot *mbp,u32 magic) {
	u32 entryPoint;

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

#if DEBUGGING
	mboot_dbg_print();
#endif

	/* mm && kheap */
	dbg_startTimer();
	vid_printf("Initializing memory-management...");
	mm_init();
	paging_mapHigherHalf();
	kheap_init();
	paging_initCOWList();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	/* vfs */
	dbg_startTimer();
	vid_printf("Initializing VFS...");
	vfs_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	/* processes */
	dbg_startTimer();
	vid_printf("Initializing process-management...");
	proc_init();
	sched_init();
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

#if DEBUGGING
	vid_printf("Free frames=%d, pages mapped=%d, free mem=%d KiB\n",
			mm_getFreeFrmCount(MM_DMA | MM_DEF),paging_dbg_getPageCount(),
			mm_getFreeFrmCount(MM_DMA | MM_DEF) * PAGE_SIZE / K);

	vfs_dbg_printTree();
#endif

#if 1
	/* TODO the following is just temporary! */
	/* load task1 */
	entryPoint = elf_loadprog(task1);
	/* give the process 2 stack pages */
	proc_changeSize(2,CHG_STACK);
	return entryPoint;
#else
	while(1);
	return 0;
#endif
}
