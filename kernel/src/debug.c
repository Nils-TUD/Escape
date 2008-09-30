/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/debug.h"
#include "../h/common.h"
#include "../h/video.h"
#include "../h/paging.h"
#include "../h/proc.h"
#include "../h/cpu.h"

static u64 start = 0;

void dbg_startTimer(void) {
	start = cpu_rdtsc();
}

void dbg_stopTimer(void) {
	u64 diff = cpu_rdtsc() - start;
	u32 *ptr = &diff;
	vid_printf("Clock cycles: 0x%08x%08x\n",*(ptr + 1),*ptr);
}

void dbg_printProcess(tProc *p) {
	vid_printf("process @ 0x%08x:\n",p);
	vid_printf("\tpid = %d\n",p->pid);
	vid_printf("\tparentPid = %d\n",p->parentPid);
	vid_printf("\tphysPDirAddr = 0x%08x\n",p->physPDirAddr);
	vid_printf("\ttextPages = %d\n",p->textPages);
	vid_printf("\tdataPages = %d\n",p->dataPages);
	vid_printf("\tstackPages = %d\n",p->stackPages);
	vid_printf("\n");
}

void dbg_printPageDir(void) {
	u32 i;
	tPDEntry *pagedir = (tPDEntry*)PAGE_DIR_AREA;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pagedir[i].present) {
			dbg_printPageTable(i,pagedir[i].ptFrameNo,(tPTEntry*)(MAPPED_PTS_START + i * PAGE_SIZE));
		}
	}
	vid_printf("\n");
}

void dbg_printPageTable(u32 no,u32 frame,tPTEntry *pagetable) {
	u32 i;
	u32 addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	vid_printf("\tpagetable 0x%x [frame 0x%x] @ 0x%08x: (VM: 0x%08x - 0x%08x)\n",no,frame,pagetable,
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
		vid_printf("raw=0x%08x, frame=0x%x [%c%c]",*(u32*)page,
				page->frameNumber,page->notSuperVisor ? 'u' : 'k',page->writable ? 'w' : 'r');
	}
	else {
		vid_printf("-");
	}
}
