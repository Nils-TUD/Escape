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
	u32 *ptr = (u32*)&diff;
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
	dbg_printProcessState(&p->save);
	vid_printf("\n");
}

void dbg_printProcessState(tProcSave *state) {
	vid_printf("\tprocessState @ 0x%08x:\n",state);
	vid_printf("\t\tesp = 0x%08x\n",state->esp);
	vid_printf("\t\tedi = 0x%08x\n",state->edi);
	vid_printf("\t\tesi = 0x%08x\n",state->esi);
	vid_printf("\t\tebp = 0x%08x\n",state->ebp);
	vid_printf("\t\teflags = 0x%08x\n",state->eflags);
}

void dbg_printPageDir(bool includeKernel) {
	u32 i;
	tPDEntry *pagedir;
	paging_mapPageDir();
	pagedir = (tPDEntry*)PAGE_DIR_AREA;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < PT_ENTRY_COUNT; i++) {
		if(pagedir[i].present && (includeKernel || i != ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR))) {
			dbg_printPageTable(i,pagedir + i);
		}
	}
	vid_printf("\n");
}

void dbg_printUserPageDir(void) {
	u32 i;
	tPDEntry *pagedir;
	paging_mapPageDir();
	pagedir = (tPDEntry*)PAGE_DIR_AREA;
	vid_printf("page-dir @ 0x%08x:\n",pagedir);
	for(i = 0; i < ADDR_TO_PDINDEX(KERNEL_AREA_V_ADDR); i++) {
		if(pagedir[i].present) {
			dbg_printPageTable(i,pagedir + i);
		}
	}
	vid_printf("\n");
}

void dbg_printPageTable(u32 no,tPDEntry *pde) {
	u32 i;
	u32 addr = PAGE_SIZE * PT_ENTRY_COUNT * no;
	tPTEntry *pte = (tPTEntry*)(MAPPED_PTS_START + no * PAGE_SIZE);
	vid_printf("\tpt 0x%x [frame 0x%x, %c%c] @ 0x%08x: (VM: 0x%08x - 0x%08x)\n",no,
			pde->ptFrameNo,pde->notSuperVisor ? 'u' : 'k',pde->writable ? 'w' : 'r',pte,addr,
			addr + (PAGE_SIZE * PT_ENTRY_COUNT) - 1);
	if(pte) {
		for(i = 0; i < PT_ENTRY_COUNT; i++) {
			if(pte[i].present) {
				vid_printf("\t\t0x%x: ",i);
				dbg_printPage(pte + i);
				vid_printf(" (VM: 0x%08x - 0x%08x)\n",addr,addr + PAGE_SIZE - 1);
			}
			addr += PAGE_SIZE;
		}
	}
}

void dbg_printPage(tPTEntry *page) {
	if(page->present) {
		vid_printf("raw=0x%08x, frame=0x%x [%c%c%c%c]",*(u32*)page,
				page->frameNumber,page->notSuperVisor ? 'u' : 'k',page->writable ? 'w' : 'r',
				page->global ? 'g' : '-',page->copyOnWrite ? 'c' : '-');
	}
	else {
		vid_printf("-");
	}
}
