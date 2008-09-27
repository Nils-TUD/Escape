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

u32 dummy = 0;

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

u32 main(tMultiBoot *mbp,u32 magic) {
	s32 i,x;
	/* the first thing we've to do is set up the page-dir and page-table for the kernel and so on
	 * and "correct" the GDT */
	paging_init();
	gdt_init();
	mboot_init(mbp);

	/* init video */
	vid_init();
	
	/*vid_printf("Magic-Number=0x%x, Multiboot-Structure=0x%08x\n",magic,mb);
	
	vid_printf("abla %:09x,%:12x,%:63c,%d,%c,%c,%x\nbla und nun\nueberschreiben\rwahh",
			0x456,0x789,'a',magic,'b','c',0x123);
	
	vid_printf("Address=0x%x\n",&dummy);
	vid_printf("%16s, %8s, %4s, %2s\n","bla","bla","bla","bla");*/
	printMultiBootInfo();
	
	/*vid_printf("Initializing GDT...");
	gdt_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");*/
	
	vid_printf("Initializing memory-management...");
	mm_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	
#ifdef TEST_MM
	test_mm();
#endif
	
	vid_printf("Initializing process-management...");
	proc_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	
	vid_printf("Initializing IDT...");
	intrpt_init();
	vid_toLineEnd(vid_getswidth("DONE"));
	vid_printf("%:02s","DONE");
	
	/*u32 frames[5];
	mm_allocateFrames(MM_DEF,frames,5);
	paging_map(procs[pi].pageDir,(s8*)0xB0000000,frames,5,PG_WRITABLE);
	
	dbg_printPageDir(procs[pi].pageDir);*/
	
	/* jetzt wo wir schon im Kernel drin sind, wollen wir auch nicht mehr raus ;) */
	while (1);
	return 0;
}
