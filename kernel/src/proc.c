/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/proc.h"
#include "../h/paging.h"
#include "../h/mm.h"
#include "../h/util.h"
#include "../h/intrpt.h"
#include "../h/sched.h"
#include "../h/vfs.h"
#include "../h/vfsinfo.h"
#include "../h/kheap.h"
#include "../h/gdt.h"
#include "../h/cpu.h"
#include "../h/video.h"
#include <string.h>

/* our processes */
static sProc procs[PROC_COUNT];
/* TODO keep that? */
/* the process-index */
static tPid pi;

/* pointer to a dead proc that has to be deleted */
static sProc *deadProc = NULL;

void proc_init(void) {
	tFD i;
	/* init the first process */
	pi = 0;
	if(!vfs_createProcess(0,&vfsinfo_procReadHandler))
		panic("Not enough mem for init process");
	procs[pi].state = ST_RUNNING;
	procs[pi].pid = 0;
	procs[pi].parentPid = 0;
	/* the first process has no text, data and stack */
	procs[pi].textPages = 0;
	procs[pi].dataPages = 0;
	procs[pi].stackPages = 0;
	procs[pi].cycleCount = 0;
	/* note that this assumes that the page-dir is initialized */
	procs[pi].physPDirAddr = (u32)paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		procs[pi].fileDescs[i] = -1;
	/* TODO just temporary */
	memcpy(procs[pi].name,"init",5);

	paging_exchangePDir(procs[pi].physPDirAddr);
	/* setup kernel-stack for us */
	paging_map(KERNEL_STACK,NULL,1,PG_WRITABLE | PG_SUPERVISOR,false);
}

tPid proc_getFreePid(void) {
	tPid pid;
	/* we can skip our initial process */
	for(pid = 1; pid < PROC_COUNT; pid++) {
		if(procs[pid].state == ST_UNUSED)
			return pid;
	}
	return 0;
}

sProc *proc_getRunning(void) {
	return &procs[pi];
}

sProc *proc_getByPid(tPid pid) {
	ASSERT(pid < PROC_COUNT,"pid >= PROC_COUNT");
	return &procs[pid];
}

void proc_switch(void) {
	proc_switchTo(sched_perform()->pid);
}

void proc_switchTo(tPid pid) {
	static u64 startTime = 0;
	sProc *p = procs + pi;

	if(startTime > 0)
		p->cycleCount += cpu_rdtsc() - startTime;

	if(pid != pi && !proc_save(&p->save)) {
		pi = pid;
		p = procs + pi;
		sched_setRunning(p);
		/* remove the io-map. it will be set as soon as the process accesses an io-port
		 * (we'll get an exception) */
		tss_removeIOMap();
		startTime = cpu_rdtsc();
		proc_resume(p->physPDirAddr,&p->save);
	}

	/* destroy process, if there is any */
	if(deadProc != NULL) {
		proc_destroy(deadProc);
		deadProc = NULL;
	}
}

s32 proc_requestIOPorts(u16 start,u16 count) {
	sProc *p = procs + pi;
	u16 end;
	if(p->ioMap == NULL) {
		p->ioMap = (u8*)kheap_alloc(IO_MAP_SIZE / 8);
		if(p->ioMap == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	end = start + count;
	/* 0xF8 .. 0xFF is reserved */
	if((start >= 0xF8 && start <= 0xFF) ||	/* start in range */
		(end > 0xF8 && end <= 0x100) ||		/* end in range */
		(start < 0xF8 && end > 0x100))		/* range between start and end */
		return ERR_IO_MAP_RANGE_RESERVED;

	while(count-- > 0) {
		p->ioMap[start / 8] &= ~(1 << (start % 8));
		start++;
	}

	/* refresh io-map */
	tss_setIOMap(p->ioMap);

	return 0;
}

s32 proc_releaseIOPorts(u16 start,u16 count) {
	sProc *p = procs + pi;
	u16 end;
	if(p->ioMap == NULL)
		return ERR_IOMAP_NOT_PRESENT;

	end = start + count;
	/* 0xF8 .. 0xFF is reserved */
	if((start >= 0xF8 && start <= 0xFF) ||	/* start in range */
		(end > 0xF8 && end <= 0x100) ||		/* end in range */
		(start < 0xF8 && end > 0x100))		/* range between start and end */
		return ERR_IO_MAP_RANGE_RESERVED;

	while(count-- > 0) {
		p->ioMap[start / 8] |= 1 << (start % 8);
		start++;
	}

	/* refresh io-map */
	tss_setIOMap(p->ioMap);

	return 0;
}

tFile proc_fdToFile(tFD fd) {
	tFile fileNo;
	if(fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = (procs + pi)->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	return fileNo;
}

s32 proc_getFreeFd(void) {
	tFD i;
	tFile *fds = procs[pi].fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1)
			return i;
	}

	return ERR_MAX_PROC_FDS;
}

s32 proc_assocFd(tFD fd,tFile fileNo) {
	if(fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	if(procs[pi].fileDescs[fd] != -1)
		return ERR_INVALID_FD;

	procs[pi].fileDescs[fd] = fileNo;
	return 0;
}

s32 proc_dupFd(tFD fd) {
	tFile f;
	s32 nfd;
	/* check fd */
	if(fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	f = procs[pi].fileDescs[fd];
	if(f == -1)
		return ERR_INVALID_FD;

	nfd = proc_getFreeFd();
	if(nfd < 0)
		return nfd;

	procs[pi].fileDescs[nfd] = f;
	return 0;
}

s32 proc_redirFd(tFD src,tFD dst) {
	tFile fSrc,fDst;
	/* check fds */
	if(src >= MAX_FD_COUNT || dst >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fSrc = procs[pi].fileDescs[src];
	fDst = procs[pi].fileDescs[dst];
	if(fSrc == -1 || fDst == -1)
		return ERR_INVALID_FD;

	/* we have to close the source because no one else will do it anymore... */
	vfs_closeFile(fSrc);

	/* now redirect src to dst */
	procs[pi].fileDescs[src] = fDst;
	return 0;
}

tFile proc_unassocFD(tFD fd) {
	tFile fileNo;
	if(fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = procs[pi].fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	procs[pi].fileDescs[fd] = -1;
	return fileNo;
}

s32 proc_clone(tPid newPid) {
	u32 i,pdirFrame,stackFrame;
	u32 *src,*dst;
	sProc *p;

	ASSERT(newPid < PROC_COUNT,"newPid >= PROC_COUNT");
	ASSERT((procs + newPid)->state == ST_UNUSED,"The process slot 0x%x is already in use!",
			procs + newPid);

	/* first create the VFS node (we may not have enough mem) */
	if(!vfs_createProcess(newPid,&vfsinfo_procReadHandler))
		return -1;

	/* clone page-dir */
	if((pdirFrame = paging_clonePageDir(&stackFrame,procs + newPid)) == 0)
		return -1;

	/* set page-dir and pages for segments */
	p = procs + newPid;
	p->pid = newPid;
	p->parentPid = pi;
	p->textPages = procs[pi].textPages;
	p->dataPages = procs[pi].dataPages;
	p->stackPages = procs[pi].stackPages;
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	p->cycleCount = 0;

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = procs[pi].fileDescs[i];
		if(p->fileDescs[i] != -1)
			p->fileDescs[i] = vfs_inheritFile(newPid,p->fileDescs[i]);
	}

	/* make ready */
	sched_setReady(p);

	/* map stack temporary (copy later) */
	paging_map(KERNEL_STACK_TMP,&stackFrame,1,PG_SUPERVISOR | PG_WRITABLE,true);

	/* save us first */
	if(proc_save(&p->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent function-call (otherwise we would change the stack) */
	src = (u32*)KERNEL_STACK;
	dst = (u32*)KERNEL_STACK_TMP;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;
	/* unmap it */
	paging_unmap(KERNEL_STACK_TMP,1,false);

	/* parent */
	return 0;
}

void proc_suicide(void) {
	if(pi == 0)
		panic("The initial process has to be alive!!");

	/* mark ourself as destroyable */
	deadProc = procs + pi;
	/* ensure that we will not be selected on the next resched */
	deadProc->state = ST_ZOMBIE;
}

void proc_destroy(sProc *p) {
	tFD i;
	/* don't delete initial or unused processes */
	ASSERT(p->pid != 0 && p->state != ST_UNUSED,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);
	ASSERT(p->pid != pi,"You can't destroy yourself!");

	/* destroy paging-structure */
	paging_destroyPageDir(p);

	/* release file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vfs_closeFile(p->fileDescs[i]);
			p->fileDescs[i] = -1;
		}
	}

	/* remove from VFS */
	vfs_removeProcess(p->pid);

	/* TODO we have to unregister services, if p is on */

	/* mark as unused */
	p->textPages = 0;
	p->dataPages = 0;
	p->stackPages = 0;
	/* remove from scheduler */
	sched_removeProc(p);
	p->state = ST_UNUSED;
	p->pid = 0;
	p->physPDirAddr = 0;
}

void proc_setupIntrptStack(sIntrptStackFrame *frame) {
	ASSERT(frame != NULL,"frame == NULL");

	frame->uesp = KERNEL_AREA_V_ADDR - sizeof(u32);
	frame->ebp = frame->uesp;
	/* user-mode segments */
	/* TODO remove the hard-coded stuff here! */
	frame->cs = 0x1b;
	frame->ds = 0x23;
	frame->es = 0x23;
	frame->fs = 0x23;
	frame->gs = 0x23;
	frame->uss = 0x23;
	/* TODO entry-point */
	frame->eip = 0;
	/* general purpose register */
	frame->eax = 0;
	frame->ebx = 0;
	frame->ecx = 0;
	frame->edx = 0;
	frame->esi = 0;
	frame->edi = 0;
}

bool proc_segSizesValid(u32 textPages,u32 dataPages,u32 stackPages) {
	u32 maxPages = KERNEL_AREA_V_ADDR / PAGE_SIZE;
	return textPages + dataPages + stackPages <= maxPages;
}

bool proc_changeSize(s32 change,eChgArea area) {
	u32 addr,chg = change,origPages;

	ASSERT(area == CHG_DATA || area== CHG_STACK,"area invalid");

	/* determine start-address */
	if(area == CHG_DATA) {
		addr = (procs[pi].textPages + procs[pi].dataPages) * PAGE_SIZE;
		origPages = procs[pi].textPages + procs[pi].dataPages;
	}
	else {
		addr = KERNEL_AREA_V_ADDR - (procs[pi].stackPages + change) * PAGE_SIZE;
		origPages = procs[pi].stackPages;
	}

	if(change > 0) {
		u32 ts,ds,ss;
		/* not enough mem? */
		if(mm_getFreeFrmCount(MM_DEF) < paging_countFramesForMap(addr,change)) {
			return false;
		}
		/* invalid segment sizes? */
		ts = procs[pi].textPages;
		ds = procs[pi].dataPages;
		ss = procs[pi].stackPages;
		if((area == CHG_DATA && !proc_segSizesValid(ts,ds + change,ss))
				|| (area == CHG_STACK && !proc_segSizesValid(ts,ds,ss + change))) {
			return false;
		}

		paging_map(addr,NULL,change,PG_WRITABLE,false);

		/* now clear the memory */
		while(change-- > 0) {
			memset((void*)addr,0,PAGE_SIZE);
			addr += PAGE_SIZE;
		}
	}
	else {
		/* we have to correct the address */
		addr += change * PAGE_SIZE;

		/* free and unmap memory */
		paging_unmap(addr,-change,true);

		/* can we remove all page-tables? */
		if(origPages + change == 0) {
			paging_unmapPageTables(ADDR_TO_PDINDEX(addr),PAGES_TO_PTS(-change));
		}
		/* ok, remove just the free ones */
		else {
			/* at first calculate the max. pts we may free (based on the pages we removed) */
			s32 start = ADDR_TO_PDINDEX(addr & ~(PAGE_SIZE * PT_ENTRY_COUNT - 1));
			s32 count = ADDR_TO_PDINDEX(addr - (change + 1) * PAGE_SIZE) - ADDR_TO_PDINDEX(addr) + 1;
			/* don't delete the first pt? */
			if(area == CHG_DATA && (addr & (PAGE_SIZE * PT_ENTRY_COUNT - 1)) > 0) {
				start++;
				count--;
			}
			/* don't delete last pt? */
			else if(area == CHG_STACK &&
					((addr - change * PAGE_SIZE) & (PAGE_SIZE * PT_ENTRY_COUNT - 1)) > 0) {
				count--;
			}

			/* finally unmap the pts */
			if(count > 0)
				paging_unmapPageTables(start,count);
		}
	}

	/* adjust sizes */
	if(area == CHG_DATA) {
		procs[pi].dataPages += chg;
	}
	else {
		procs[pi].stackPages += chg;
	}

	return true;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void proc_dbg_printAll(void) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].state != ST_UNUSED)
			proc_dbg_print(procs + i);
	}
}

void proc_dbg_print(sProc *p) {
	u32 i;
	u32 *ptr;
	vid_printf("process @ 0x%08x:\n",p);
	vid_printf("\tpid = %d\n",p->pid);
	vid_printf("\tparentPid = %d\n",p->parentPid);
	vid_printf("\tphysPDirAddr = 0x%08x\n",p->physPDirAddr);
	vid_printf("\ttextPages = %d\n",p->textPages);
	vid_printf("\tdataPages = %d\n",p->dataPages);
	vid_printf("\tstackPages = %d\n",p->stackPages);
	ptr = (u32*)&p->cycleCount;
	vid_printf("\tcycleCount = 0x%08x%08x\n",*(ptr + 1),*ptr);
	vid_printf("\tfileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1)
			vid_printf("\t\t%d : %d\n",i,p->fileDescs[i]);
	}
	proc_dbg_printState(&p->save);
	vid_printf("\n");
}

void proc_dbg_printState(sProcSave *state) {
	vid_printf("\tprocessState @ 0x%08x:\n",state);
	vid_printf("\t\tesp = 0x%08x\n",state->esp);
	vid_printf("\t\tedi = 0x%08x\n",state->edi);
	vid_printf("\t\tesi = 0x%08x\n",state->esi);
	vid_printf("\t\tebp = 0x%08x\n",state->ebp);
	vid_printf("\t\teflags = 0x%08x\n",state->eflags);
}

#endif
