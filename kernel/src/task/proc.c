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

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/signals.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/sharedmem.h>
#include <sys/machine/intrpt.h>
#include <sys/machine/fpu.h>
#include <sys/machine/gdt.h>
#include <sys/machine/cpu.h>
#include <sys/machine/timer.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/node.h>
#include <sys/util.h>
#include <sys/syscalls.h>
#include <sys/video.h>
#include <esc/sllist.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

static void proc_notifyProcDied(tPid parent,tPid pid);
static s32 proc_finishClone(sThread *nt,u32 stackFrame);
static bool proc_setupThreadStack(sIntrptStackFrame *frame,void *arg,u32 tentryPoint);
static u32 *proc_addStartArgs(sThread *t,u32 *esp,u32 tentryPoint,bool newThread);

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
	p->pagedir = paging_getProc0PD() & ~KERNEL_AREA_V_ADDR;
	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(0,&vfsinfo_procReadHandler);
	vassert(p->threadDir != NULL,"Not enough mem for init process");

	p->pid = 0;
	p->parentPid = 0;
	/* 1 pagedir, 1 page-table for kernel-stack, 1 for kernelstack */
	p->ownFrames = 1 + 1 + 1;
	/* the first process has no text, data and stack */
	p->swapped = 0;
	p->exitCode = 0;
	p->exitSig = SIG_COUNT;
	p->sigRetAddr = 0;
	p->flags = 0;
	memcpy(p->command,"initloader",11);

	/* create first thread */
	p->threads = sll_create();
	vassert(p->threads != NULL,"Unable to create initial thread-list");
	vassert(sll_append(p->threads,thread_init(p)),"Unable to append the initial thread");

	paging_exchangePDir(p->pagedir);
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

sProc *proc_getProcWithBin(sBinDesc *bin,tVMRegNo *rno) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			tVMRegNo res = vmm_hasBinary(procs + i,bin);
			if(res != -1) {
				*rno = res;
				return procs + i;
			}
		}
	}
	return NULL;
}

sRegion *proc_getLRURegion(void) {
	u32 i,ts = ULONG_MAX;
	sRegion *lru = NULL;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID && i != ATA_PID) {
			sRegion *reg = vmm_getLRURegion(procs + i);
			if(reg && reg->timestamp < ts) {
				ts = reg->timestamp;
				lru = reg;
			}
		}
	}
	return lru;
}

void proc_getMemUsage(u32 *paging,u32 *data) {
	u32 pmem = 0,dmem = 0;
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			u32 ptmp,dtmp;
			vmm_getMemUsage(procs + i,&ptmp,&dtmp);
			dmem += dtmp;
			pmem += ptmp;
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

s32 proc_clone(tPid newPid,bool isVM86) {
	u32 dummy,stackFrame;
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

	p->swapped = 0;

	/* clone page-dir */
	if((res = paging_cloneKernelspace(&stackFrame,&p->pagedir)) < 0)
		goto errorVFS;
	p->ownFrames = res;

	/* set basic attributes */
	p->pid = newPid;
	p->parentPid = cur->pid;
	p->exitCode = 0;
	p->exitSig = SIG_COUNT;
	p->sigRetAddr = cur->sigRetAddr;
	if(isVM86)
		p->flags |= P_VM86;
	/* give the process the same name (may be changed by exec) */
	strcpy(p->command,cur->command);
	/* clone regions */
	p->regions = NULL;
	p->regSize = 0;
	if((res = vmm_cloneAll(p)) < 0)
		goto errorPDir;

	/* clone shared-memory-regions */
	if((res = shm_cloneProc(cur,p)) < 0)
		goto errorRegs;

	/* create thread-list */
	p->threads = sll_create();
	if(p->threads == NULL)
		goto errorShm;

	/* clone current thread */
	if((res = thread_clone(curThread,&nt,p,&dummy,true)) < 0)
		goto errorThreadList;
	if(!sll_append(p->threads,nt))
		goto errorThread;
	/* set kernel-stack-frame; thread_clone() doesn't do it for us */
	nt->kstackFrame = stackFrame;
	/* make thread ready */
	thread_setReady(nt->tid);

	res = proc_finishClone(nt,stackFrame);
	if(res == 1) {
		/* child */
		return 1;
	}
	/* parent */
	return 0;

errorThread:
	thread_kill(nt);
errorThreadList:
	kheap_free(p->threads);
errorShm:
	shm_remProc(p);
errorRegs:
	proc_removeRegions(p,true);
errorPDir:
	paging_destroyPDir(p->pagedir);
errorVFS:
	vfs_removeProcess(newPid);
	return ERR_NOT_ENOUGH_MEM;
}

s32 proc_startThread(u32 entryPoint,void *arg) {
	u32 stackFrame;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	sThread *nt;
	s32 res;
	if((res = thread_clone(t,&nt,t->proc,&stackFrame,false)) < 0)
		return res;

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		thread_kill(nt);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* mark ready */
	thread_setReady(nt->tid);

	res = proc_finishClone(nt,stackFrame);
	if(res == 1) {
		sIntrptStackFrame *istack = intrpt_getCurStack();
		if(!proc_setupThreadStack(istack,arg,entryPoint)) {
			thread_kill(nt);
			/* do a switch here because we can't continue */
			thread_switch();
			/* never reached */
			return ERR_NOT_ENOUGH_MEM;
		}
		proc_setupStart(istack,entryPoint);

		/* child */
		return 0;
	}

	return nt->tid;
}

static s32 proc_finishClone(sThread *nt,u32 stackFrame) {
	u32 i,*src;
	/* we clone just the current thread. all other threads are ignored */
	/* map stack temporary (copy later) */
	u32 *dst = (u32*)paging_mapToTemp(&stackFrame,1);

	if(thread_save(&nt->save)) {
		/* child */
		return 1;
	}

	/* now copy the stack */
	/* copy manually to prevent a function-call (otherwise we would change the stack) */
	src = (u32*)KERNEL_STACK;
	for(i = 0; i < PT_ENTRY_COUNT; i++)
		*dst++ = *src++;

	paging_unmapFromTemp(1);

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
		thread_kill(t);
	}
}

void proc_removeRegions(sProc *p,bool remStack) {
	u32 i;
	sSLNode *n;
	assert(p);
	/* remove from shared-memory; do this first because it will remove the region and simply
	 * assumes that the region still exists. */
	shm_remProc(p);
	for(i = 0; i < p->regSize; i++) {
		sVMRegion *vm = ((sVMRegion**)p->regions)[i];
		if(vm && (!(vm->reg->flags & RF_STACK) || remStack))
			vmm_remove(p,i);
	}
	/* unset TLS-region (and stack-region, if needed) from all threads */
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		t->tlsRegion = -1;
		if(remStack)
			t->stackRegion = -1;
		/* remove all signal-handler since we've removed the code to handle signals */
		sig_removeHandlerFor(t->tid);
	}
}

s32 proc_getExitState(tPid ppid,sExitState *state) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID && procs[i].parentPid == ppid) {
			if(procs[i].flags & P_ZOMBIE) {
				if(state) {
					state->pid = procs[i].pid;
					state->exitCode = procs[i].exitCode;
					state->signal = procs[i].exitSig;
					/* TODO */
					state->memory = 0;/*(procs[i].textPages + procs[i].dataPages +
							procs[i].stackPages) * PAGE_SIZE;*/
					state->ucycleCount.val64 = 0;
					state->kcycleCount.val64 = 0;
					/* TODO
					for(n = sll_begin(procs[i].threads); n != NULL; n = n->next) {
						t = (sThread*)n->data;
						state->ucycleCount.val64 += t->ucycleCount.val64;
						state->kcycleCount.val64 += t->kcycleCount.val64;
					}*/
				}
				return procs[i].pid;
			}
		}
	}
	return ERR_NO_CHILD;
}

void proc_terminate(sProc *p,s32 exitCode,tSig signal) {
	sSLNode *tn,*tmpn;
	vassert(p->pid != 0 && p->pid != INVALID_PID,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);

	/* if its already a zombie and we don't want to kill ourself, kill the process */
	if((p->flags & P_ZOMBIE) && p != proc_getRunning()) {
		proc_kill(p);
		return;
	}

	/* remove all threads */
	for(tn = sll_begin(p->threads); tn != NULL; ) {
		sThread *t = (sThread*)tn->data;
		tmpn = tn->next;
		thread_kill(t);
		tn = tmpn;
	}

	/* remove all regions */
	proc_removeRegions(p,true);

	/* free io-map, if present */
	if(p->ioMap != NULL) {
		kheap_free(p->ioMap);
		p->ioMap = NULL;
	}

	/* store exit-conditions */
	p->exitCode = exitCode;
	p->exitSig = signal;
	p->flags |= P_ZOMBIE;

	proc_notifyProcDied(p->parentPid,p->pid);
}

void proc_kill(sProc *p) {
	u32 i;
	sProc *cp;
	sProc *cur = proc_getRunning();
	vassert(p->pid != 0 && p->pid != INVALID_PID,
			"The process @ 0x%x with pid=%d is unused or the initial process",p,p->pid);
	vassert(p != cur,"We can't kill the current process!");

	/* give childs the ppid 0 */
	cp = procs;
	for(i = 0; i < PROC_COUNT; i++) {
		if(cp->pid != INVALID_PID && cp->parentPid == p->pid) {
			cp->parentPid = 0;
			/* if this process has already died, the parent can't wait for it since its dying
			 * right now. therefore notify init of it */
			if(cp->flags & P_ZOMBIE)
				proc_notifyProcDied(0,cp->pid);
		}
		cp++;
	}

	/* remove pagedir; TODO we can do that on terminate, too (if we delay it when its the current) */
	paging_destroyPDir(p->pagedir);
	/* destroy threads-list */
	sll_destroy(p->threads,false);

	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* notify processes that wait for dying procs */
	sig_addSignal(SIG_PROC_DIED,p->pid);
	sig_addSignalFor(p->parentPid,SIG_CHILD_DIED,p->pid);

	/* mark as unused */
	p->pid = INVALID_PID;
	p->pagedir = 0;
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

bool proc_setupUserStack(sIntrptStackFrame *frame,u32 argc,char *args,u32 argsSize,
		sStartupInfo *info) {
	u32 *esp;
	char **argv;
	u32 totalSize;
	sThread *t = thread_getRunning();
	vassert(frame != NULL,"frame == NULL");

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |     arguments    |
	 * |        ...       |
	 * +------------------+
	 * |       argv       |
	 * +------------------+
	 * |       argc       |
	 * +------------------+
	 * |     phdrSize     |  the size of the program-headers (for eh)
	 * +------------------+
	 * |       phdr       |  the address of the program-headers (for eh)
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* we need to know the total number of bytes we'll store on the stack */
	totalSize = 0;
	if(argc > 0) {
		/* first round the size of the arguments up. then we need argc+1 pointer */
		totalSize += (argsSize + sizeof(u32) - 1) & ~(sizeof(u32) - 1);
		totalSize += sizeof(u32) * (argc + 1);
	}
	/* finally we need argc, argv, phdrSize, phdr, tlsSize, tlsStart and entryPoint */
	totalSize += sizeof(u32) * 7;

	/* get esp */
	vmm_getRegRange(t->proc,t->stackRegion,NULL,(u32*)&esp);

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
	*esp-- = argc;
	*esp-- = info->phdrSize;
	*esp-- = info->phdr;
	/* add TLS args and entrypoint; use prog-entry here because its always the entry of the
	 * program, not the dynamic-linker */
	esp = proc_addStartArgs(t,esp,info->progEntry,false);

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
	/* gs is used for TLS */
	frame->gs = SEGSEL_GDTI_UTLS | SEGSEL_RPL_USER | SEGSEL_TI_GDT;
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

static bool proc_setupThreadStack(sIntrptStackFrame *frame,void *arg,u32 tentryPoint) {
	u32 *esp;
	u32 totalSize = 3 * sizeof(u32) + sizeof(void*);
	sThread *t = thread_getRunning();

	/*
	 * Initial stack:
	 * +------------------+  <- top
	 * |       arg        |
	 * +------------------+
	 * |     TLSSize      |  0 if not present
	 * +------------------+
	 * |     TLSStart     |  0 if not present
	 * +------------------+
	 * |    entryPoint    |  0 for initial thread, thread-entrypoint for others
	 * +------------------+
	 */

	/* get esp */
	vmm_getRegRange(t->proc,t->stackRegion,NULL,(u32*)&esp);

	/* extend the stack if necessary */
	if(thread_extendStack((u32)esp - totalSize) < 0)
		return false;
	/* will handle copy-on-write */
	paging_isRangeUserWritable((u32)esp - totalSize,totalSize);

	/* put arg on stack */
	esp--;
	*esp-- = (u32)arg;
	/* add TLS args and entrypoint */
	esp = proc_addStartArgs(t,esp,tentryPoint,true);

	frame->uesp = (u32)esp;
	frame->ebp = frame->uesp;
	return true;
}

static u32 *proc_addStartArgs(sThread *t,u32 *esp,u32 tentryPoint,bool newThread) {
	/* put address and size of the tls-region on the stack */
	if(t->tlsRegion >= 0) {
		u32 tlsStart,tlsEnd;
		vmm_getRegRange(t->proc,t->tlsRegion,&tlsStart,&tlsEnd);
		*esp-- = tlsEnd - tlsStart;
		*esp-- = tlsStart;
	}
	else {
		/* no tls */
		*esp-- = 0;
		*esp-- = 0;
	}

	*esp = newThread ? tentryPoint : 0;
	return esp;
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

void proc_dbg_printAllPDs(u8 parts,bool regions) {
	u32 i;
	for(i = 0; i < PROC_COUNT; i++) {
		if(procs[i].pid != INVALID_PID) {
			vid_printf("Process %d (%s) (%d own, %d sh, %d sw):\n",
					procs[i].pid,procs[i].command,procs[i].ownFrames,procs[i].sharedFrames,
					procs[i].swapped);
			if(regions)
				vmm_dbg_print(procs + i);
			paging_dbg_printPDir(procs[i].pagedir,parts);
			vid_printf("\n");
		}
	}
}

void proc_dbg_print(sProc *p) {
	sSLNode *n;
	vid_printf("proc %d:\n",p->pid);
	vid_printf("\tppid=%d, cmd=%s, pdir=%x\n",p->parentPid,p->command,p->pagedir);
	vid_printf("\townFrames=%u, sharedFrames=%u\n",p->ownFrames,p->sharedFrames);
	vid_printf("\tswapped=%u\n",p->swapped);
	if(p->threads) {
		for(n = sll_begin(p->threads); n != NULL; n = n->next)
			thread_dbg_print((sThread*)n->data);
	}
	vid_printf("\n");
}

#endif
