/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <multiboot.h>
#include <gdt.h>
#include <mm.h>
#include <util.h>
#include <paging.h>
#include <proc.h>
#include <intrpt.h>
#include <debug.h>
#include <cpu.h>
#include <elf.h>
#include <kheap.h>
#include <elf.h>
#include <sched.h>
#include <vfs.h>
#include <vfsinfo.h>
#include <video.h>
#include <timer.h>

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

static u8 initloader[] = {
	#include "../../build/user_initloader.dump"
};

s32 main(sMultiBoot *mbp,u32 magic) {
	u32 entryPoint;

	UNUSED(magic);

	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video */
	vid_init();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

#if DEBUGGING
	mboot_dbg_print();
#endif

	/* mm */
	vid_printf("Initializing physical memory-management...");
	mm_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* paging */
	vid_printf("Initializing paging...");
	paging_mapHigherHalf();
	paging_initCOWList();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* vfs */
	vid_printf("Initializing VFS...");
	vfs_init();
	vfsinfo_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* processes */
	vid_printf("Initializing process-management...");
	proc_init();
	sched_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* idt */
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* timer */
	vid_printf("Initializing Timer...");
	timer_init();
	vid_toLineEnd(strlen("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

#if DEBUGGING
	vid_printf("Free frames=%d, pages mapped=%d, free mem=%d KiB\n",
			mm_getFreeFrmCount(MM_DMA | MM_DEF),paging_dbg_getPageCount(),
			mm_getFreeFrmCount(MM_DMA | MM_DEF) * PAGE_SIZE / K);
#endif

	/* load initloader */
	entryPoint = elf_loadprog(initloader);
	/* give the process 2 stack pages */
	proc_changeSize(2,CHG_STACK);
	return entryPoint;
}
