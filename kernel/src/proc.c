/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <common.h>
#include <proc.h>
#include <paging.h>
#include <mm.h>
#include <util.h>
#include <intrpt.h>
#include <fpu.h>
#include <sched.h>
#include <vfs.h>
#include <vfsinfo.h>
#include <kheap.h>
#include <gdt.h>
#include <cpu.h>
#include <video.h>
#include <signals.h>
#include <vfsnode.h>
#include <string.h>
#include <sllist.h>
#include <assert.h>
#include <errors.h>

/* our processes */
static sProc procs[PROC_COUNT];
/* the process-index */
static tPid pi;

/* list of dead processes that should be destroyed */
static sSLList* deadProcs = NULL;

void proc_init(void) {
	tFD i;
	/* init the first process */
	pi = 0;
	if(!vfs_createProcess(0,&vfsinfo_procReadHandler))
		util_panic("Not enough mem for init process");
	procs[pi].state = ST_RUNNING;
	procs[pi].events = EV_NOEVENT;
	procs[pi].pid = 0;
	procs[pi].parentPid = 0;
	/* the first process has no text, data and stack */
	procs[pi].textPages = 0;
	procs[pi].dataPages = 0;
	procs[pi].stackPages = 0;
	procs[pi].ucycleCount = 0;
	procs[pi].ucycleStart = 0;
	procs[pi].kcycleCount = 0;
	procs[pi].kcycleStart = 0;
	procs[pi].fpuState = NULL;
	procs[pi].text = NULL;
	/* note that this assumes that the page-dir is initialized */
	procs[pi].physPDirAddr = (u32)paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		procs[pi].fileDescs[i] = -1;
	memcpy(procs[pi].command,"initloader",5);

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
	return INVALID_PID;
}

sProc *proc_getRunning(void) {
	return &procs[pi];
}

sProc *proc_getByPid(tPid pid) {
	vassert(pid < PROC_COUNT,"pid >= PROC_COUNT");
	return &procs[pid];
}

bool proc_exists(tPid pid) {
	return pid < PROC_COUNT && procs[pid].state != ST_UNUSED;
}

bool proc_hasChild(tPid pid) {
	tPid i;
	sProc *p = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		if(p->state != ST_UNUSED && p->parentPid == pid)
			return true;
		p++;
	}
	return false;
}

void proc_switch(void) {
	proc_switchTo(sched_perform()->pid);
}

void proc_switchTo(tPid pid) {
	sProc *p = procs + pi;

	/* finish kernel-time here since we're switching the process */
	if(p->kcycleStart > 0)
		p->kcycleCount += cpu_rdtsc() - p->kcycleStart;

	if(pid != pi && !proc_save(&p->save)) {
		/* mark old process ready, if it should not be blocked, killed or something */
		if(p->state == ST_RUNNING)
			sched_setReady(p);

		pi = pid;
		p = procs + pi;
		sched_setRunning(p);
		/* remove the io-map. it will be set as soon as the process accesses an io-port
		 * (we'll get an exception) */
		tss_removeIOMap();
		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		fpu_lockFPU();
		proc_resume(p->physPDirAddr,&p->save);
	}

	/* now start kernel-time again */
	proc_getRunning()->kcycleStart = cpu_rdtsc();

	/* destroy process, if there is any */
	if(deadProcs != NULL) {
		sSLNode *n;
		for(n = sll_begin(deadProcs); n != NULL; n = n->next)
			proc_destroy((sProc*)n->data);
		sll_destroy(deadProcs,false);
		deadProcs = NULL;
	}
}

void proc_wait(tPid pid,u8 events) {
	sProc *p = procs + pid;
	p->events = events;
	sched_setBlocked(p);
}

void proc_wakeupAll(u8 event) {
	sched_unblockAll(event);
}

void proc_wakeup(tPid pid,u8 event) {
	sProc *p = procs + pid;
	if(p->events & event) {
		/* ensure that the process is the next one that will be chosen */
		sched_setReadyQuick(p);
		p->events = EV_NOEVENT;
	}
}

s32 proc_requestIOPorts(u16 start,u16 count) {
	sProc *p = procs + pi;
	if(p->ioMap == NULL) {
		p->ioMap = (u8*)kheap_alloc(IO_MAP_SIZE / 8);
		if(p->ioMap == NULL)
			return ERR_NOT_ENOUGH_MEM;
	}

	/* 0xF8 .. 0xFF is reserved */
	if(OVERLAPS(0xF8,0xFF + 1,start,start + count))
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
	if(p->ioMap == NULL)
		return ERR_IOMAP_NOT_PRESENT;

	/* 0xF8 .. 0xFF is reserved */
	if(OVERLAPS(0xF8,0xFF + 1,start,start + count))
		return ERR_IO_MAP_RANGE_RESERVED;

	while(count-- > 0) {
		p->ioMap[start / 8] |= 1 << (start % 8);
		start++;
	}

	/* refresh io-map */
	tss_setIOMap(p->ioMap);

	return 0;
}

tFileNo proc_fdToFile(tFD fd) {
	tFileNo fileNo;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = (procs + pi)->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	return fileNo;
}

tFD proc_getFreeFd(void) {
	tFD i;
	tFileNo *fds = procs[pi].fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1)
			return i;
	}

	return ERR_MAX_PROC_FDS;
}

s32 proc_assocFd(tFD fd,tFileNo fileNo) {
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	if(procs[pi].fileDescs[fd] != -1)
		return ERR_INVALID_FD;

	procs[pi].fileDescs[fd] = fileNo;
	return 0;
}

tFD proc_dupFd(tFD fd) {
	tFileNo f;
	s32 err,nfd;
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	f = procs[pi].fileDescs[fd];
	if(f == -1)
		return ERR_INVALID_FD;

	nfd = proc_getFreeFd();
	if(nfd < 0)
		return nfd;

	/* increase references */
	if((err = vfs_incRefs(f)) < 0)
		return err;

	procs[pi].fileDescs[nfd] = f;
	return nfd;
}

s32 proc_redirFd(tFD src,tFD dst) {
	tFileNo fSrc,fDst;
	s32 err;

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fSrc = procs[pi].fileDescs[src];
	fDst = procs[pi].fileDescs[dst];
	if(fSrc == -1 || fDst == -1)
		return ERR_INVALID_FD;

	if((err = vfs_incRefs(fDst)) < 0)
		return err;

	/* we have to close the source because no one else will do it anymore... */
	vfs_closeFile(fSrc);

	/* now redirect src to dst */
	procs[pi].fileDescs[src] = fDst;
	return 0;
}

tFileNo proc_unassocFD(tFD fd) {
	tFileNo fileNo;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = procs[pi].fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	procs[pi].fileDescs[fd] = -1;
	return fileNo;
}

s32 proc_extendStack(u32 address) {
	sProc *p = procs + pi;
	s32 newPages;
	address &= ~(PAGE_SIZE - 1);
	newPages = ((KERNEL_AREA_V_ADDR - address) >> PAGE_SIZE_SHIFT) - p->stackPages;
	if(newPages > 0) {
		if(!proc_segSizesValid(p->textPages,p->dataPages,p->stackPages + newPages))
			return ERR_NOT_ENOUGH_MEM;
		if(!proc_changeSize(newPages,CHG_STACK))
			return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 proc_clone(tPid newPid) {
	u32 i,pdirFrame,stackFrame;
	u32 *src,*dst;
	sProc *p;

	vassert(newPid < PROC_COUNT,"newPid >= PROC_COUNT");
	vassert((procs + newPid)->state == ST_UNUSED,"The process slot 0x%x is already in use!",
			procs + newPid);

	/* first create the VFS node (we may not have enough mem) */
	if(!vfs_createProcess(newPid,&vfsinfo_procReadHandler))
		return ERR_NOT_ENOUGH_MEM;

	/* clone page-dir */
	if((pdirFrame = paging_clonePageDir(&stackFrame,procs + newPid)) == 0)
		return ERR_NOT_ENOUGH_MEM;

	/* set page-dir and pages for segments */
	p = procs + newPid;
	p->events = EV_NOEVENT;
	p->pid = newPid;
	p->parentPid = pi;
	p->textPages = procs[pi].textPages;
	p->dataPages = procs[pi].dataPages;
	p->stackPages = procs[pi].stackPages;
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	p->ucycleCount = 0;
	p->ucycleStart = 0;
	p->kcycleCount = 0;
	p->kcycleStart = 0;
	p->fpuState = NULL;
	/* give the process the same name (maybe changed by exec) */
	strcpy(p->command,procs[pi].command);

	/* clone text */
	p->text = procs[pi].text;
	text_clone(p->text,p->pid);

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = procs[pi].fileDescs[i];
		if(p->fileDescs[i] != -1)
			p->fileDescs[i] = vfs_inheritFileNo(newPid,p->fileDescs[i]);
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
	paging_unmap(KERNEL_STACK_TMP,1,false,false);

	/* parent */
	return 0;
}

void proc_destroy(sProc *p) {
	tFD i;
	sProc *cp;
	/* don't delete initial or unused processes */
	vassert(p->pid != 0 && p->state != ST_UNUSED,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);

	/* we can't destroy ourself so we mark us as dead and do this later */
	if(p->pid == pi) {
		/* create list if not already done */
		if(deadProcs == NULL) {
			deadProcs = sll_create();
			if(deadProcs == NULL)
				util_panic("Not enough mem for deadProcs");
		}

		/* mark ourself as destroyable */
		sll_append(deadProcs,procs + pi);
		/* ensure that we will not be selected on the next resched */
		procs[pi].state = ST_ZOMBIE;
		return;
	}

	/* destroy paging-structure */
	paging_destroyPageDir(p);

	/* release file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vfs_closeFile(p->fileDescs[i]);
			p->fileDescs[i] = -1;
		}
	}

	/* give childs the ppid 0 */
	cp = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		/* TODO use parent id of parent? */
		if(cp->state != ST_UNUSED && cp->parentPid == p->pid)
			cp->parentPid = 0;
		cp++;
	}

	/* free io-map, if present */
	if(p->ioMap != NULL)
		kheap_free(p->ioMap);

	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* remove signals */
	sig_removeHandlerFor(p->pid);
	/* notify processes that wait for dying procs */
	sig_addSignal(SIG_PROC_DIED,p->pid);
	/* free FPU-state-memory */
	fpu_freeState(&p->fpuState);

	/* mark as unused */
	p->textPages = 0;
	p->dataPages = 0;
	p->stackPages = 0;
	/* remove from scheduler */
	sched_removeProc(p);
	p->state = ST_UNUSED;
	p->pid = 0;
	p->physPDirAddr = 0;

	/* notify parent, if waiting */
	proc_wakeup(p->parentPid,EV_CHILD_DIED);
}

void proc_setupIntrptStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize) {
	u32 *esp;
	char **argv;
	u32 totalSize;
	vassert(frame != NULL,"frame == NULL");

	/* we need to know the total number of bytes we'll store on the stack */
	totalSize = 0;
	if(argc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += (argsSize + sizeof(u32) - 1) & ~(sizeof(u32) - 1);
		totalSize += sizeof(u32) * (argc + 1);
	}
	/* finally we need argc and argv itself */
	totalSize += sizeof(u32) * 2;

	/* will handle copy-on-write */
	/* TODO we might have to add stack-pages... */
	paging_isRangeUserWritable(KERNEL_AREA_V_ADDR - totalSize,totalSize);

	/* copy arguments on the user-stack */
	esp = (u32*)KERNEL_AREA_V_ADDR - 1;
	argv = NULL;
	if(argc > 0) {
		char *str;
		u32 i,len;
		argv = (char**)(esp - argc);
		/* space for the argument-pointer */
		esp -= argc;
		/* start for the arguments */
		str = (char*)esp;
		for(i = 0; i < argc; i++) {
			/* start <len> bytes backwards */
			len = strlen(args) + 1;
			str -= len;
			/* store arg-pointer and copy arg */
			argv[i] = str;
			memcpy(str,args,len);
			/* to next */
			args += len;
		}
		/* ensure that we don't overwrites the characters */
		esp = (u32*)(((u32)str & ~(sizeof(u32) - 1)) - sizeof(u32));
	}

	/* store argc and argv */
	*esp-- = (u32)argv;
	*esp = argc;

	frame->uesp = (u32)esp;
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

	vassert(area == CHG_DATA || area== CHG_STACK,"area invalid");

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
		if(mm_getFreeFrmCount(MM_DEF) < paging_countFramesForMap(addr,change))
			return false;

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
		memset((void*)addr,0,PAGE_SIZE * change);
	}
	else {
		/* we have to correct the address */
		addr += change * PAGE_SIZE;

		/* free and unmap memory */
		paging_unmap(addr,-change,true,true);

		/* can we remove all page-tables? */
		if(origPages + change == 0)
			paging_unmapPageTables(ADDR_TO_PDINDEX(addr),PAGES_TO_PTS(-change));
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
	if(area == CHG_DATA)
		procs[pi].dataPages += chg;
	else
		procs[pi].stackPages += chg;

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
	const char *states[] = {"UNUSED","RUNNING","READY","BLOCKED","ZOMBIE"};
	u32 i;
	u32 *ptr;
	vid_printf("process @ 0x%08x:\n",p);
	vid_printf("\tpid = %d\n",p->pid);
	vid_printf("\tparentPid = %d\n",p->parentPid);
	vid_printf("\tcommand = %s\n",p->command);
	vid_printf("\tstate = %s\n",states[p->state]);
	vid_printf("\tphysPDirAddr = 0x%08x\n",p->physPDirAddr);
	vid_printf("\ttextPages = %d\n",p->textPages);
	vid_printf("\tdataPages = %d\n",p->dataPages);
	vid_printf("\tstackPages = %d\n",p->stackPages);
	ptr = (u32*)&p->ucycleCount;
	vid_printf("\tucycleCount = 0x%08x%08x\n",*(ptr + 1),*ptr);
	ptr = (u32*)&p->kcycleCount;
	vid_printf("\tkcycleCount = 0x%08x%08x\n",*(ptr + 1),*ptr);
	vid_printf("\tfileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			tVFSNodeNo no = vfs_getNodeNo(p->fileDescs[i]);
			vid_printf("\t\t%d : %d",i,p->fileDescs[i]);
			if(vfsn_isValidNodeNo(no)) {
				sVFSNode *n = vfsn_getNode(no);
				if(n && n->parent)
					vid_printf(" (%s->%s)",n->parent->name,n->name);
			}
			vid_printf("\n");
		}
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
