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
	
	dbg_startTimer();
	vid_printf("Initializing memory-management...");
	mm_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	vid_printf("Free frames=%d, pages mapped=%d\n",mm_getNumberOfFreeFrames(MM_DMA | MM_DEF),
			paging_getPageCount());

	dbg_startTimer();
	vid_printf("Initializing process-management...");
	proc_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

	dbg_startTimer();
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	dbg_stopTimer();

#if 0
esp = 0xc0110f8c
ebp = 0xc0110fe8
0xc01010a0 <isrCommon+14>:	call   0xc0101520 <intrpt_handler>

esp = 0xc0110f88
ebp = 0xc0110fe8
0xc0101520 <intrpt_handler+0>:	push   %ebp

esp = 0xc0110f84
ebp = 0xc0110fe8
0xc0101521 <intrpt_handler+1>:	mov    %esp,%ebp

esp = 0xc0110f84
ebp = 0xc0110f84
0xc0101523 <intrpt_handler+3>:	movl   $0x123,0xc(%ebp)
0xc010152a <intrpt_handler+10>:	movl   $0xc01052a4,0x8(%ebp)
0xc0101531 <intrpt_handler+17>:	pop    %ebp

esp = 0xc0110f88
ebp = 0xc0110fe8
0xc0101532 <intrpt_handler+18>:	jmp    0xc0101510 <varargs>

0xc0101510 <varargs+0>:	push   %ebp

esp = 0xc0110f84
ebp = 0xc0110fe8
0xc0101511 <varargs+1>:	mov    %esp,%ebp

esp = 0xc0110f84
ebp = 0xc0110f84
0xc0101513 <varargs+3>:	sub    $0x10,%esp
0xc0101516 <varargs+6>:	mov    0xc(%ebp),%eax
0xc0101519 <varargs+9>:	leave  

esp = 0xc0110f88
ebp = 0xc0110fe8
0xc010151a <varargs+10>:	ret

esp = 0xc0110f8c
ebp = 0xc0110fe8
#endif
	
	
#ifdef TEST_MM
	test_mm();
#endif
#ifdef TEST_PAGING
	test_paging();
#endif
#ifdef TEST_PROC
	test_proc();
#endif
	
	*(u32*)0x4000 = 1;
	
	while(1);
	return 0;
}
