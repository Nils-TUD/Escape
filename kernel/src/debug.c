/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/debug.h"
#include "../h/common.h"
#include "../h/video.h"
#include "../h/paging.h"

void dbg_printPageDir(tPDEntry *pagedir) {
	u32 i;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pagedir[i].present) {
			dbg_printPageTable(i,pagedir[i].ptAddress,(tPTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE));
		}
	}
	vid_printf("\n");
}

void dbg_printPageTable(u32 no,u32 frame,tPTEntry *pagetable) {
	u32 i;
	u32 addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	vid_printf("\tpagetable 0x%x [frame %x] @ 0x%08x: (VM: 0x%08x - 0x%08x)\n",no,frame,pagetable,
			addr,addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pagetable) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pagetable[i].present) {
				vid_printf("\t\t0x%x: ",i);
				dbg_printPage(pagetable + i);
				vid_printf(" (VM: 0x%08x - 0x%08x)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

void dbg_printPage(tPTEntry *page) {
	if(page->present) {
		vid_printf("raw=%x, frame=%x [%c%c]",*(u32*)page,page->frameNumber,page->notSuperVisor ? 'u' : 'k',
				page->writable ? 'w' : 'r');
	}
	else {
		vid_printf("-");
	}
}
