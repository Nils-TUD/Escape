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
#include <mem/vmm.h>
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
#include <limits.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

#define EXIT_CALL_ADDR					(TEXT_BEGIN + 0xf)

static void proc_notifyProcDied(tPid parent,tPid pid);
static s32 proc_finishClone(sThread *nt,u32 stackFrame);

/* our processes */
static tPid nextPid = 1;
static sProc procs[PROC_COUNT];

void proc_init(void) {
	/* init the first process */
	sProc *p = procs + 0;
	tPid pid;
	sThread *t;
	u32 stackFrame;

	/* do this first because vfs_createProcess may use the kheap so that the kheap needs paging
	 * and paging refers to the current process's pagedir */
	p->physPDirAddr = (u32)paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(0,&vfsinfo_procReadHandler);
	vassert(p->threadDir != NULL,"Not enough mem for init process");

	p->pid = 0;
	p->parentPid = 0;
	/* 1 pagedir, 1 page-table for kernel-stack, 1 for kernelstack */
	p->frameCount = 1 + 1 + 1;
	/* the first process has no text, data and stack */
	p->swapped = 0;
	p->unswappable = 0;
	p->exitCode = 0;
	p->exitSig = SIG_COUNT;
	p->isVM86 = false;
	memcpy(p->command,"initloader",11);

	/* create first thread */
	p->threads = sll_create();
	vassert(p->threads != NULL,"Unable to create initial thread-list");
	vassert(sll_append(p->threads,thread_init(p)),"Unable to append the initial thread");

	paging_exchangePDir(p->physPDirAddr);
	/* setup kernel-stack for us */
	stackFrame = mm_allocateFrame(MM_DEF);
	paging_map(KERNEL_STACK,&stackFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);

	/* set kernel-stack for first thread */
	t = (sThread*)sll_get(p->threads,0);
	t->kstackFrame = stackFrame;

	/* mark all other processes unused */
	for(pid = 1; pid < PROC_COUNT; pid++)
		procs[pid].pid = INVALID_PID;
}

tPid proc_getFreePid(void) {
	tPid pid;
	/* start with the first possibly usable pid */
	for(pid = nextPid; pid < PROC_COUNT; pid++) {
		if(procs[pid].pid == INVALID_PID) {
			/* go to the next one; note that its ok here to use pid 0 since its occupied anyway */
			nextPid = (pid + 1) % PROC_COUNT;
			return pid;
		}
	}
	/* if we're here all pids starting from nextPid to PROC_COUNT - 1 are in use. so start at the
	 * beginning */
	for(pid = 1; pid < nextPid; pid++) {
		if(procs[pid].pid == INVALID_PID) {
			nextPid = (pid + 1) % PROC_COUNT;
			return pid;
		}
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

sProc *proc_getLRUProc(void) {
	u32 i;
	sProc *cur = proc_getRunning();
	sProc *lru = NULL;
	u64 lruTime = LLONG_MAX;
	for(i = 0; i < PROC_COUNT; i++) {
		/* we don't want to swap the current one */
		if(procs[i].pid != INVALID_PID && procs + i != cur && proc_getSwapCount(procs + i) > 0) {
			sSLNode *n;
			u64 time = 0;
			/* find last used thread */
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				sThread *t = (sThread*)n->data;
				/* don't swap out ata because ata performs the swapping ;) */
				if(t->tid == ATA_TID) {
					time = LLONG_MAX;
					break;
				}
				if(t->lastSched > time)
					time = t->lastSched;
			}

			/* if it was used before the current one, its our LRU */
			if(time < lruTime) {
				lruTime = time;
				lru = procs + i;
			}
		}
	}
	return lru;
}

u32 proc_getSwapCount(sProc *p) {
	assert(p->pid != INVALID_PID);
	/* TODO */
	return 0/*(p->textPages + p->dataPages + p->stackPages - p->unswappable) - p->swapped*/;
}

void proc_getMemUsageOf(sProc *p,u32 *paging,u32 *data) {
	sSLNode *n;
	sThread *t;
	/* TODO */
	*paging = 0;
	*data = 0;
#if 0
	u32 pmem = 0,dmem = 0;
	dmem += p->dataPages + p->textPages;
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		t = (sThread*)n->data;
		dmem += t->ustackPages;
		pmem += PAGES_TO_PTS(t->ustackPages);
	}
	/* page-directory, pt for kernel-stack and kernel-stack */
	pmem += 3 + PAGES_TO_PTS(p->textPages + p->dataPages);
	*paging = pmem * PAGE_SIZE;
	*data = dmem * PAGE_SIZE;
#endif
}

void proc_getMemUsage(u32 *paging,u32 *data) {
	u32 pmem = 0,dmem = 0;
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			u32 ptmp,dtmp;
			proc_getMemUsageOf(procs + i,&ptmp,&dtmp);
			dmem += dtmp;
			pmem += ptmp;
		}
	}
	*paging = pmem;
	*data = dmem;
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

s32 proc_clone(tPid newPid,bool isVM86) {
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

	/* unswappable may be changed in clonePageDir() */
	p->swapped = 0;
	p->unswappable = 0;

	/* clone page-dir */
	if((pdirFrame = paging_cloneKernelspace(&stackFrame)) == 0)
		goto errorVFS;

	/* clone regions */
	p->pid = newPid;
	/* give the process the same name (maybe changed by exec) */
	strcpy(p->command,cur->command);
	p->regions = NULL;
	p->regSize = 0;
	/* required for cloning */
	p->physPDirAddr = pdirFrame << PAGE_SIZE_SHIFT;
	if((res = vmm_cloneAll(p)) < 0)
		goto errorPDir;

	/* set page-dir and pages for segments */
	p->parentPid = cur->pid;
	/* 1 pagedir, 1 page-table for kernel-stack, 1 for kernelstack */
	p->frameCount = 1 + 1 + 1;
	p->exitCode = 0;
	p->exitSig = SIG_COUNT;
	p->isVM86 = isVM86;

	/* create thread-list */
	p->threads = sll_create();
	if(p->threads == NULL)
		goto errorRegs;

	/* clone current thread */
	if((res = thread_clone(curThread,&nt,p,&dummy,true)) < 0)
		goto errorThreadList;
	if(!sll_append(p->threads,nt))
		goto errorThread;
	/* set kernel-stack-frame; thread_clone() doesn't do it for us */
	nt->kstackFrame = stackFrame;
	/* make thread ready */
	sched_setReady(nt);

	res = proc_finishClone(nt,stackFrame);
	if(res == 1)
		return 1;
	/* parent */
	return 0;

errorThread:
	p->frameCount -= thread_destroy(nt,true);
errorThreadList:
	kheap_free(p->threads);
errorRegs:
	vmm_removeAll(p,true);
errorPDir:
	paging_destroyPDir(p->physPDirAddr);
errorVFS:
	vfs_removeProcess(newPid);
	return ERR_NOT_ENOUGH_MEM;
}

s32 proc_startThread(u32 entryPoint,s32 argc,char *args,u32 argSize) {
	u32 stackFrame;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	sThread *nt;
	s32 res;
	if((res = thread_clone(t,&nt,t->proc,&stackFrame,false)) < 0)
		return res;

	p->frameCount += res;

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		p->frameCount -= thread_destroy(nt,true);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* mark ready */
	sched_setReady(nt);

	res = proc_finishClone(nt,stackFrame);
	if(res == 1) {
		u32 *esp;
		sIntrptStackFrame *istack = intrpt_getCurStack();
		if(!proc_setupUserStack(istack,argc,args,argSize))
			goto error;
		proc_setupStart(istack,entryPoint);

		/* we want to call exit when the thread-function returns */
		esp = (u32*)istack->uesp;
		if(thread_extendStack((u32)esp - sizeof(u32)) < 0)
			goto error;
		*--esp = EXIT_CALL_ADDR;
		istack->uesp = (u32)esp;

		/* child */
		return 0;
	}

	return nt->tid;

error:
	p->frameCount -= thread_destroy(nt,true);
	/* do a switch here because we can't continue */
	thread_switch();
	/* never reached */
	return ERR_NOT_ENOUGH_MEM;
}

static s32 proc_finishClone(sThread *nt,u32 stackFrame) {
	u32 i,*src,*dst;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	paging_map(TEMP_MAP_PAGE,&stackFrame,1,PG_PRESENT | PG_SUPERVISOR | PG_WRITABLE);

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
	paging_unmap(TEMP_MAP_PAGE,1,false);

	/* parent */
	return 0;
}

void proc_destroyThread(s32 exitCode) {
	sProc *cur = proc_getRunning();
	/* last thread? */
	if(sll_length(cur->threads) == 1)
		proc_terminate(cur,exitCode,SIG_COUNT);
	else {
		/* just destroy the thread */
		sThread *t = thread_getRunning();
		cur->frameCount -= thread_destroy(t,true);
	}
}

s32 proc_getExitState(tPid ppid,sExitState *state) {
	u32 i;
	sThread *t;
	sSLNode *n;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID && procs[i].parentPid == ppid) {
			bool isZombie = true;
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				if(((sThread*)n->data)->state != ST_ZOMBIE) {
					isZombie = false;
					break;
				}
			}
			if(isZombie) {
				if(state) {
					state->pid = procs[i].pid;
					state->exitCode = procs[i].exitCode;
					state->signal = procs[i].exitSig;
					/* TODO */
					state->memory = 0;/*(procs[i].textPages + procs[i].dataPages +
							procs[i].stackPages) * PAGE_SIZE;*/
					state->ucycleCount.val64 = 0;
					state->kcycleCount.val64 = 0;
					for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
						t = (sThread*)n->data;
						state->ucycleCount.val64 += t->ucycleCount.val64;
						state->kcycleCount.val64 += t->kcycleCount.val64;
					}
				}
				return procs[i].pid;
			}
		}
	}
	return ERR_NO_CHILD;
}

void proc_terminate(sProc *p,s32 exitCode,tSig signal) {
	sSLNode *tn;
	vassert(p->pid != 0 && p->pid != INVALID_PID,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);

	/* ensure that no thread of our process will be selected on the next resched */
	for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
		sThread *t = (sThread*)tn->data;
		sched_removeThread(t);
		/* ensure that we don't get a signal anymore */
		sig_removeHandlerFor(t->tid);
		t->state = ST_ZOMBIE;
	}

	/* store exit-conditions */
	p->exitCode = exitCode;
	p->exitSig = signal;

	proc_notifyProcDied(p->parentPid,p->pid);
}

void proc_kill(sProc *p) {
	u32 i;
	sSLNode *tn,*tmpn;
	sProc *cp;
	sProc *cur = proc_getRunning();
	vassert(p->pid != 0 && p->pid != INVALID_PID,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);
	vassert(p->pid != cur->pid,"We can't kill the current process!");

	/* remove all regions and page-dir */
	vmm_removeAll(p,true);
	p->frameCount -= paging_destroyPDir(p->physPDirAddr);

	/* destroy threads */
	for(tn = sll_begin(p->threads); tn != NULL; ) {
		sThread *t = (sThread*)tn->data;
		tmpn = tn->next;
		p->frameCount -= thread_destroy(t,false);
		tn = tmpn;
	}
	sll_destroy(p->threads,false);

	/* give childs the ppid 0 */
	cp = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		if(cp->pid != INVALID_PID && cp->parentPid == p->pid) {
			cp->parentPid = 0;
			/* if this process has already died, the parent can't wait for it since its dying
			 * right now. therefore notify init of it */
			if(((sThread*)sll_begin(cp->threads)->data)->state == ST_ZOMBIE)
				proc_notifyProcDied(0,cp->pid);
		}
		cp++;
	}

	/* free io-map, if present */
	if(p->ioMap != NULL)
		kheap_free(p->ioMap);

	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* notify processes that wait for dying procs */
	sig_addSignal(SIG_PROC_DIED,p->pid);
	sig_addSignalFor(p->parentPid,SIG_CHILD_DIED,p->pid);

	/* mark as unused */
	p->pid = INVALID_PID;
	p->physPDirAddr = 0;
}

static void proc_notifyProcDied(tPid parent,tPid pid) {
	sSLNode *tn;
	sig_addSignalFor(parent,SIG_CHILD_TERM,pid);

	/* check whether there is a parent-thread that waits for a child */
	for(tn = sll_begin(proc_getByPid(parent)->threads); tn != NULL; tn = tn->next) {
		sThread *t = (sThread*)tn->data;
		if(t->events & EV_CHILD_DIED) {
			/* wake the thread and delay the destruction until this read has got the exit-state */
			thread_wakeup(t->tid,EV_CHILD_DIED);
			return;
		}
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

		/* check whether the string is readable */
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

bool proc_setupUserStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize) {
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

	/* get esp */
	vmm_getRegRange(thread->proc,thread->stackRegion,NULL,(u32*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((u32)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((u32)esp - totalSize,totalSize);

	/* copy arguments on the user-stack (4byte space) */
	esp--;
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
	return true;
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


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

static u64 ucycles[PROC_COUNT];
static u64 kcycles[PROC_COUNT];

void proc_dbg_startProf(void) {
	u32 i;
	sSLNode *n;
	sThread *t;
	for(i = 0; i < PROC_COUNT; i++) {
		ucycles[i] = 0;
		kcycles[i] = 0;
		if(procs[i].pid != INVALID_PID) {
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				t = (sThread*)n->data;
				ucycles[i] += t->ucycleCount.val64;
				kcycles[i] += t->kcycleCount.val64;
			}
		}
	}
}

void proc_dbg_stopProf(void) {
	u32 i;
	sSLNode *n;
	sThread *t;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			uLongLong curUcycles;
			uLongLong curKcycles;
			curUcycles.val64 = 0;
			curKcycles.val64 = 0;
			for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
				t = (sThread*)n->data;
				curUcycles.val64 += t->ucycleCount.val64;
				curKcycles.val64 += t->kcycleCount.val64;
			}
			curUcycles.val64 -= ucycles[i];
			curKcycles.val64 -= kcycles[i];
			if(curUcycles.val64 > 0 || curKcycles.val64 > 0) {
				vid_printf("Process %3d (%18s): u=%08x%08x k=%08x%08x\n",
					procs[i].pid,procs[i].command,
					curUcycles.val32.upper,curUcycles.val32.lower,
					curKcycles.val32.upper,curKcycles.val32.lower);
			}
		}
	}
}

void proc_dbg_printAll(void) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID)
			proc_dbg_print(procs + i);
	}
}

void proc_dbg_printAllPDs(u8 parts) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			vid_printf("Process %d (%s):\n",procs[i].pid,procs[i].command);
			paging_dbg_printPDir(procs[i].physPDirAddr,parts);
			vid_printf("\n");
		}
	}
}

void proc_dbg_print(sProc *p) {
	sSLNode *n;
	vid_printf("proc %d:\n",p->pid);
	vid_printf("\tppid=%d, cmd=%s, pdir=%x\n",p->parentPid,p->command,p->physPDirAddr);
	vid_printf("\tunswappable=%d, swapped=%d\n",p->unswappable,p->swapped);
	for(n = sll_begin(p->threads); n != NULL; n = n->next)
		thread_dbg_print((sThread*)n->data);
	vid_printf("\n");
}

#endif
