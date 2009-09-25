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
#include <task/proc.h>
#include <task/thread.h>
#include <task/signals.h>
#include <task/sched.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <mem/kheap.h>
#include <machine/intrpt.h>
#include <machine/fpu.h>
#include <machine/gdt.h>
#include <machine/cpu.h>
#include <vfs/vfs.h>
#include <vfs/info.h>
#include <vfs/node.h>
#include <util.h>
#include <syscalls.h>
#include <video.h>
#include <string.h>
#include <sllist.h>
#include <assert.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

#define EXIT_CALL_ADDR	0xf

static s32 proc_finishClone(sThread *nt,u32 stackFrame);

/* our processes */
static sProc procs[PROC_COUNT];

/* list of dead processes that should be destroyed */
static sSLList* deadProcs = NULL;

void proc_init(void) {
	/* init the first process */
	sProc *p = procs + 0;
	tPid pid;
	sThread *t;
	u32 stackFrame;

	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(0,&vfsinfo_procReadHandler);
	vassert(p->threadDir != NULL,"Not enough mem for init process");

	p->pid = 0;
	p->parentPid = 0;
	/* the first process has no text, data and stack */
	p->textPages = 0;
	p->dataPages = 0;
	p->stackPages = 0;
	p->text = NULL;
	/* note that this assumes that the page-dir is initialized */
	p->physPDirAddr = (u32)paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
	memcpy(p->command,"initloader",11);

	/* create first thread */
	p->threads = sll_create();
	vassert(p->threads != NULL,"Unable to create initial thread-list");
	vassert(sll_append(p->threads,thread_init(p)),"Unable to append the initial thread");

	paging_exchangePDir(p->physPDirAddr);
	/* setup kernel-stack for us */
	stackFrame = mm_allocateFrame(MM_DEF);
	paging_map(KERNEL_STACK,&stackFrame,1,PG_WRITABLE | PG_SUPERVISOR,false);

	/* set kernel-stack for first thread */
	t = (sThread*)sll_get(p->threads,0);
	t->kstackFrame = stackFrame;

	/* mark all other processes unused */
	for(pid = 1; pid < PROC_COUNT; pid++)
		procs[pid].pid = INVALID_PID;
}

tPid proc_getFreePid(void) {
	tPid pid;
	/* we can skip our initial process */
	for(pid = 1; pid < PROC_COUNT; pid++) {
		if(procs[pid].pid == INVALID_PID)
			return pid;
	}
	return INVALID_PID;
}

sProc *proc_getRunning(void) {
	sThread *t = thread_getRunning();
	if(t)
		return t->proc;
	return &procs[0];
}

sProc *proc_getByPid(tPid pid) {
	vassert(pid < PROC_COUNT,"pid >= PROC_COUNT");
	return &procs[pid];
}

bool proc_exists(tPid pid) {
	return pid < PROC_COUNT && procs[pid].pid != INVALID_PID;
}

u32 proc_getCount(void) {
	u32 i,count = 0;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID)
			count++;
	}
	return count;
}

void proc_getMemUsage(u32 *paging,u32 *data) {
	sSLNode *n;
	sThread *t;
	u32 pmem = 0,dmem = 0;
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			dmem += procs[i].dataPages + procs[i].textPages;
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				t = (sThread*)n->data;
				dmem += t->ustackPages;
				pmem += PAGES_TO_PTS(t->ustackPages);
			}
			/* page-directory, pt for kernel-stack and kernel-stack */
			pmem += 3 + PAGES_TO_PTS(procs[i].textPages + procs[i].dataPages);
		}
	}
	*paging = pmem * PAGE_SIZE;
	*data = dmem * PAGE_SIZE;
}

bool proc_hasChild(tPid pid) {
	tPid i;
	sProc *p = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		if(p->pid != INVALID_PID && p->parentPid == pid)
			return true;
		p++;
	}
	return false;
}

void proc_cleanup(void) {
	/* destroy process, if there is any */
	if(deadProcs != NULL) {
		sSLNode *n;
		for(n = sll_begin(deadProcs); n != NULL; n = n->next)
			proc_destroy((sProc*)n->data);
		sll_destroy(deadProcs,false);
		deadProcs = NULL;
	}
}

s32 proc_clone(tPid newPid) {
	u32 pdirFrame,dummy,stackFrame;
	sProc *p;
	sProc *cur = proc_getRunning();
	sThread *curThread = thread_getRunning();
	sThread *nt;
	s32 res;

	vassert(newPid < PROC_COUNT,"newPid >= PROC_COUNT");
	vassert((procs + newPid)->pid == INVALID_PID,"The process slot 0x%x is already in use!",
			procs + newPid);

	p = procs + newPid;
	/* first create the VFS node (we may not have enough mem) */
	p->threadDir = vfs_createProcess(newPid,&vfsinfo_procReadHandler);
	if(p->threadDir == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* clone page-dir */
	if((pdirFrame = paging_clonePageDir(&stackFrame,procs + newPid)) == 0) {
		vfs_removeProcess(newPid);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* set page-dir and pages for segments */
	p->pid = newPid;
	/* set text for paging_destroyPageDir() */
	p->text = NULL;
	p->parentPid = cur->pid;
	p->textPages = cur->textPages;
	p->dataPages = cur->dataPages;
	p->stackPages = curThread->ustackPages;
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	/* give the process the same name (maybe changed by exec) */
	strcpy(p->command,cur->command);

	/* create thread-list */
	p->threads = sll_create();
	if(p->threads == NULL) {
		paging_destroyPageDir(p);
		vfs_removeProcess(newPid);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* clone current thread */
	if((res = thread_clone(curThread,&nt,p,&dummy,true)) < 0) {
		kheap_free(p->threads);
		paging_destroyPageDir(p);
		vfs_removeProcess(newPid);
		return res;
	}
	if(!sll_append(p->threads,nt)) {
		thread_destroy(nt,true);
		kheap_free(p->threads);
		paging_destroyPageDir(p);
		vfs_removeProcess(newPid);
		return ERR_NOT_ENOUGH_MEM;
	}
	/* set kernel-stack-frame; thread_clone() doesn't do it for us */
	nt->kstackFrame = stackFrame;
	/* make thread ready */
	sched_setReady(nt);

	/* clone text */
	p->text = cur->text;
	if(!text_clone(p->text,p->pid)) {
		thread_destroy(nt,true);
		kheap_free(p->threads);
		paging_destroyPageDir(p);
		vfs_removeProcess(newPid);
		return ERR_NOT_ENOUGH_MEM;
	}

	res = proc_finishClone(nt,stackFrame);
	if(res == 1)
		return 1;
	/* parent */
	return 0;
}

s32 proc_startThread(u32 entryPoint) {
	u32 stackFrame;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	sThread *nt;
	s32 res;
	if((res = thread_clone(t,&nt,t->proc,&stackFrame,false)) < 0)
		return res;

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		thread_destroy(nt,true);
		return ERR_NOT_ENOUGH_MEM;
	}

	p->stackPages += nt->ustackPages;

	/* mark ready */
	sched_setReady(nt);

	res = proc_finishClone(nt,stackFrame);
	if(res == 1) {
		u32 *esp;
		sIntrptStackFrame *istack = intrpt_getCurStack();
		proc_setupStart(istack,entryPoint);

		/* we want to call exit when the thread-function returns */
		esp = (u32*)nt->ustackBegin - 1;
		*--esp = EXIT_CALL_ADDR;
		istack->uesp = (u32)esp;
		istack->ebp = (u32)esp;

		/* child */
		return 0;
	}

	return nt->tid;
}

static s32 proc_finishClone(sThread *nt,u32 stackFrame) {
	u32 i,*src,*dst;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	paging_map(TEMP_MAP_PAGE,&stackFrame,1,PG_SUPERVISOR | PG_WRITABLE,true);

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (u32*)KERNEL_STACK;
	dst = (u32*)TEMP_MAP_PAGE;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	/* unmap it */
	paging_unmap(TEMP_MAP_PAGE,1,false,false);

	/* parent */
	return 0;
}

void proc_destroyThread(void) {
	sProc *cur = proc_getRunning();
	/* last thread? */
	if(sll_length(cur->threads) == 1)
		proc_destroy(cur);
	else {
		/* just destroy the thread */
		sThread *t = thread_getRunning();
		thread_destroy(t,true);
	}
}

void proc_destroy(sProc *p) {
	tFD i;
	sSLNode *tn,*tmpn;
	sProc *cp;
	sProc *cur = proc_getRunning();
	/* don't delete initial or unused processes */
	vassert(p->pid != 0 && p->pid != INVALID_PID,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);

	/* we can't destroy ourself so we mark us as dead and do this later */
	if(p->pid == cur->pid) {
		/* create list if not already done */
		if(deadProcs == NULL) {
			deadProcs = sll_create();
			if(deadProcs == NULL)
				util_panic("Not enough mem for deadProcs");
		}

		/* mark ourself as destroyable */
		if(!sll_append(deadProcs,procs + p->pid))
			util_panic("Not enough mem to append dead process");

		/* ensure that no thread of our process will be selected on the next resched */
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			sThread *t = (sThread*)tn->data;
			sched_removeThread(t);
			t->state = ST_ZOMBIE;
		}
		return;
	}

	/* destroy paging-structure */
	paging_destroyPageDir(p);

	/* destroy threads */
	for(tn = sll_begin(p->threads); tn != NULL; ) {
		sThread *t = (sThread*)tn->data;
		tmpn = tn->next;
		thread_destroy(t,false);
		tn = tmpn;
	}
	sll_destroy(p->threads,false);

	/* give childs the ppid 0 */
	cp = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		/* TODO use parent id of parent? */
		if(cp->pid != INVALID_PID && cp->parentPid == p->pid)
			cp->parentPid = 0;
		cp++;
	}

	/* free io-map, if present */
	if(p->ioMap != NULL)
		kheap_free(p->ioMap);

	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* notify processes that wait for dying procs */
	sig_addSignal(SIG_PROC_DIED,p->pid);

	/* mark as unused */
	p->textPages = 0;
	p->dataPages = 0;
	p->stackPages = 0;
	p->pid = INVALID_PID;
	p->physPDirAddr = 0;

	/* notify all parent-threads; TODO ok? */
	for(tn = sll_begin(proc_getByPid(p->parentPid)->threads); tn != NULL; tn = tn->next) {
		sThread *t = (sThread*)tn->data;
		thread_wakeup(t->tid,EV_CHILD_DIED);
	}
}

s32 proc_buildArgs(char **args,char **argBuffer,u32 *size,bool fromUser) {
	char **arg,*bufPos;
	u32 argc = 0;
	s32 remaining = EXEC_MAX_ARGSIZE;
	s32 len;

	/* alloc space for the arguments */
	*argBuffer = (char*)kheap_alloc(EXEC_MAX_ARGSIZE);
	if(*argBuffer == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* count args and copy them on the kernel-heap */
	/* note that we have to create a copy since we don't know where the args are. Maybe
	 * they are on the user-stack at the position we want to copy them for the
	 * new process... */
	bufPos = *argBuffer;
	arg = args;
	while(1) {
		/* check if it is a valid pointer */
		if(fromUser && !paging_isRangeUserReadable((u32)arg,sizeof(char*))) {
			kheap_free(*argBuffer);
			return ERR_INVALID_ARGS;
		}
		/* end of list? */
		if(*arg == NULL)
			break;

		/* check wether the string is readable */
		if(fromUser && !sysc_isStringReadable(*arg)) {
			kheap_free(*argBuffer);
			return ERR_INVALID_ARGS;
		}

		/* ensure that the argument is not longer than the left space */
		len = strnlen(*arg,remaining - 1);
		if(len == -1) {
			/* too long */
			kheap_free(*argBuffer);
			return ERR_INVALID_ARGS;
		}

		/* copy to heap */
		memcpy(bufPos,*arg,len + 1);
		bufPos += len + 1;
		remaining -= len + 1;
		arg++;
		argc++;
	}
	/* store args-size and return argc */
	*size = EXEC_MAX_ARGSIZE - remaining;
	return argc;
}

void proc_setupUserStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize) {
	u32 *esp;
	char **argv;
	u32 totalSize;
	sThread *thread = thread_getRunning();
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
	paging_isRangeUserWritable(thread->ustackBegin - totalSize,totalSize);

	/* copy arguments on the user-stack */
	esp = (u32*)thread->ustackBegin - 1;
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
}

void proc_setupStart(sIntrptStackFrame *frame,u32 entryPoint) {
	vassert(frame != NULL,"frame == NULL");

	/* user-mode segments */
	frame->cs = SEGSEL_GDTI_UCODE | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->ds = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->es = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->fs = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->gs = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->uss = SEGSEL_GDTI_UDATA | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
	frame->eip = entryPoint;
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
	sProc *cur = proc_getRunning();
	sThread *thread = thread_getRunning();

	vassert(area == CHG_DATA || area== CHG_STACK,"area invalid");

	/* determine start-address */
	if(area == CHG_DATA) {
		addr = (cur->textPages + cur->dataPages) * PAGE_SIZE;
		origPages = cur->textPages + cur->dataPages;
	}
	else {
		addr = thread->ustackBegin - (thread->ustackPages + change) * PAGE_SIZE;
		origPages = thread->ustackPages;
	}

	if(change > 0) {
		u32 ts,ds,ss;
		/* not enough mem? */
		if(mm_getFreeFrmCount(MM_DEF) < paging_countFramesForMap(addr,change))
			return false;

		/* invalid segment sizes? */
		ts = cur->textPages;
		ds = cur->dataPages;
		ss = cur->stackPages;
		if((area == CHG_DATA && !proc_segSizesValid(ts,ds + change,ss))
				|| (area == CHG_STACK && !proc_segSizesValid(ts,ds,ss + change))) {
			return false;
		}

		paging_map(addr,NULL,change,PG_WRITABLE,false);
		/* now clear the memory */
		memclear((void*)addr,PAGE_SIZE * change);
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
		cur->dataPages += chg;
	else {
		thread->ustackPages += chg;
		cur->stackPages += chg;
	}

	return true;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void proc_dbg_printAll(void) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID)
			proc_dbg_print(procs + i);
	}
}

void proc_dbg_print(sProc *p) {
	sSLNode *n;
	vid_printf("proc %d [ppid=%d, cmd=%s, pdir=%x, text=%d, data=%d, stack=%d]\n",p->pid,
			p->parentPid,p->command,p->physPDirAddr,p->textPages,p->dataPages,p->stackPages);
	for(n = sll_begin(p->threads); n != NULL; n = n->next)
		thread_dbg_print((sThread*)n->data);
	vid_printf("\n");
}

#endif
