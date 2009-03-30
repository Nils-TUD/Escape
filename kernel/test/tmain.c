/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
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
#include "../h/sched.h"
#include "../h/vfs.h"
#include "../h/vfsinfo.h"
#include "../h/video.h"
#include "../h/timer.h"
#include <test.h>

#include "tkheap.h"
#include "tpaging.h"
#include "tproc.h"
#include "tmm.h"
#include "tsched.h"
#include "tsllist.h"
#include "tstring.h"
#include "tvfs.h"
#include "tvfsnode.h"
#include "tsignals.h"

s32 main(sMultiBoot *mbp,u32 magic) {
	UNUSED(magic);

	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video */
	vid_init();

	vid_printf("GDT exchanged, paging enabled, video initialized");
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	mboot_dbg_print();

	/* mm */
	vid_printf("Initializing physical memory-management...");
	mm_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* paging */
	vid_printf("Initializing paging...");
	paging_mapHigherHalf();
	paging_initCOWList();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* vfs */
	vid_printf("Initializing VFS...");
	vfs_init();
	vfsinfo_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* processes */
	vid_printf("Initializing process-management...");
	proc_init();
	sched_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* idt */
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	/* timer */
	vid_printf("Initializing Timer...");
	timer_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("\033f\x2%s\033r\x0","DONE");

	vid_printf("Free frames=%d, pages mapped=%d, free mem=%d KiB\n",
			mm_getFreeFrmCount(MM_DMA | MM_DEF),paging_dbg_getPageCount(),
			mm_getFreeFrmCount(MM_DMA | MM_DEF) * PAGE_SIZE / K);


	/* start tests */
	test_register(&tModMM);
	test_register(&tModPaging);
	test_register(&tModProc);
	test_register(&tModKHeap);
	test_register(&tModSched);
	test_register(&tModSLList);
	test_register(&tModString);
	test_register(&tModVFS);
	test_register(&tModVFSn);
	test_register(&tModSignals);
	test_start();


	/* stay here */
	while(1);

	return 0;
}
