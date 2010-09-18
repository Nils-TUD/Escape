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
#include <esc/hashmap.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errors.h>

#define PROC_MAP_SIZE					1024
/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

static void proc_notifyProcDied(tPid parent,tPid pid);
static s32 proc_finishClone(sThread *nt,u32 stackFrame);
static bool proc_setupThreadStack(sIntrptStackFrame *frame,void *arg,u32 tentryPoint);
static u32 *proc_addStartArgs(sThread *t,u32 *esp,u32 tentryPoint,bool newThread);

/* we have to put the first one here because we need a process before we can allocate
 * something on the kernel-heap. sounds strange, but the kheap-init stuff uses paging and
 * this wants to have the current process. So, I guess the cleanest way is to simply put
 * the first process in the data-area (we can't free the first one anyway) */
static sProc first;
/* our processes */
static sHashMap *procs;
static sSLList *procMap[PROC_MAP_SIZE] = {NULL};
static tPid nextPid = 1;

static u32 proc_getkey(const void *data) {
	return ((sProc*)data)->pid;
}

void proc_init(void) {
	sThread *t;
	u32 stackFrame;
	
	/* init the first process */
	sProc *p = &first;

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
	p->exitState = NULL;
	p->sigRetAddr = 0;
	p->flags = 0;
	strcpy(p->command,"initloader");

	/* create first thread */
	p->threads = sll_create();
	vassert(p->threads != NULL,"Unable to create initial thread-list");
	vassert(sll_append(p->threads,thread_init(p)),"Unable to append the initial thread");

	paging_exchangePDir(p->pagedir);
	/* setup kernel-stack for us */
	stackFrame = mm_allocate();
	paging_map(KERNEL_STACK,&stackFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);

	/* set kernel-stack for first thread */
	t = (sThread*)sll_get(p->threads,0);
	t->kstackFrame = stackFrame;

	/* insert into proc-map; create the map here to ensure that the first process is initialized */
	procs = hm_create(procMap,PROC_MAP_SIZE,proc_getkey);
	assert(procs && hm_add(procs,p));
}

tPid proc_getFreePid(void) {
	u32 count = 0;
	while(count++ < MAX_PROC_COUNT) {
		if(nextPid == INVALID_PID)
			nextPid++;
		if(proc_getByPid(nextPid) == NULL)
			return nextPid++;
		nextPid++;
	}
	return INVALID_PID;
}

sProc *proc_getRunning(void) {
	sThread *t = thread_getRunning();
	if(t)
		return t->proc;
	/* just needed at the beginning */
	return &first;
}

sProc *proc_getByPid(tPid pid) {
	return hm_get(procs,pid);
}

bool proc_exists(tPid pid) {
	return hm_get(procs,pid) != NULL;
}

u32 proc_getCount(void) {
	return hm_getCount(procs);
}

sProc *proc_getProcWithBin(sBinDesc *bin,tVMRegNo *rno) {
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		tVMRegNo res = vmm_hasBinary(p,bin);
		if(res != -1) {
			*rno = res;
			return p;
		}
	}
	return NULL;
}

sRegion *proc_getLRURegion(void) {
	u32 ts = ULONG_MAX;
	sProc *p;
	sRegion *lru = NULL;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		if(p->pid != ATA_PID) {
			sRegion *reg = vmm_getLRURegion(p);
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
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		u32 ptmp,dtmp;
		vmm_getMemUsage(p,&ptmp,&dtmp);
		dmem += dtmp;
		pmem += ptmp;
	}
	*paging = pmem * PAGE_SIZE;
	*data = dmem * PAGE_SIZE;
}

bool proc_hasChild(tPid pid) {
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		if(p->parentPid == pid)
			return true;
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

	p = (sProc*)kheap_alloc(sizeof(sProc));
	if(!p)
		return ERR_NOT_ENOUGH_MEM;
	/* first create the VFS node (we may not have enough mem) */
	p->threadDir = vfs_createProcess(newPid,&vfsinfo_procReadHandler);
	if(p->threadDir == NULL)
		goto errorProc;

	p->swapped = 0;
	p->sharedFrames = 0;

	/* clone page-dir */
	if((res = paging_cloneKernelspace(&stackFrame,&p->pagedir)) < 0)
		goto errorVFS;
	p->ownFrames = res;

	/* set basic attributes */
	p->pid = newPid;
	p->parentPid = cur->pid;
	p->exitState = NULL;
	p->sigRetAddr = cur->sigRetAddr;
	p->ioMap = NULL;
	p->flags = 0;
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
	/* add to proc-map */
	if(!hm_add(procs,p))
		goto errorThread;
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
errorProc:
	kheap_free(p);
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
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		if(p->parentPid == ppid) {
			if(p->flags & P_ZOMBIE) {
				if(state) {
					if(p->exitState) {
						memcpy(state,p->exitState,sizeof(sExitState));
						kheap_free(p->exitState);
						p->exitState = NULL;
					}
				}
				return p->pid;
			}
		}
	}
	return ERR_NO_CHILD;
}

void proc_terminate(sProc *p,s32 exitCode,tSig signal) {
	sSLNode *tn,*tmpn;
	vassert(p->pid != 0,"You can't terminate the initial process");

	/* if its already a zombie and we don't want to kill ourself, kill the process */
	if((p->flags & P_ZOMBIE) && p != proc_getRunning()) {
		proc_kill(p);
		return;
	}

	/* store exit-conditions */
	p->exitState = (sExitState*)kheap_alloc(sizeof(sExitState));
	if(p->exitState) {
		p->exitState->pid = p->pid;
		p->exitState->exitCode = exitCode;
		p->exitState->signal = signal;
		p->exitState->ucycleCount.val64 = 0;
		p->exitState->kcycleCount.val64 = 0;
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			sThread *t = (sThread*)tn->data;
			p->exitState->ucycleCount.val64 += t->stats.ucycleCount.val64;
			p->exitState->kcycleCount.val64 += t->stats.kcycleCount.val64;
		}
		p->exitState->ownFrames = p->ownFrames;
		p->exitState->sharedFrames = p->sharedFrames;
		p->exitState->swapped = p->swapped;
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

	p->flags |= P_ZOMBIE;
	proc_notifyProcDied(p->parentPid,p->pid);
}

void proc_kill(sProc *p) {
	sProc *cp;
	sProc *cur = proc_getRunning();
	vassert(p->pid != 0,"You can't kill the initial process");
	vassert(p != cur,"We can't kill the current process!");

	/* give childs the ppid 0 */
	for(hm_begin(procs); (cp = hm_next(procs)); ) {
		if(cp->parentPid == p->pid) {
			cp->parentPid = 0;
			/* if this process has already died, the parent can't wait for it since its dying
			 * right now. therefore notify init of it */
			if(cp->flags & P_ZOMBIE)
				proc_notifyProcDied(0,cp->pid);
		}
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

	/* free exit-state, if not already done */
	if(p->exitState)
		kheap_free(p->exitState);

	/* remove and free */
	hm_remove(procs,p);
	kheap_free(p);
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
	/* finally we need argc, argv, tlsSize, tlsStart and entryPoint */
	totalSize += sizeof(u32) * 5;

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

#define PROF_PROC_COUNT		128

static u64 ucycles[PROF_PROC_COUNT];
static u64 kcycles[PROF_PROC_COUNT];

void proc_dbg_startProf(void) {
	sSLNode *n;
	sThread *t;
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		assert(p->pid < PROF_PROC_COUNT);
		ucycles[p->pid] = 0;
		kcycles[p->pid] = 0;
		for(n = sll_begin(p->threads); n != NULL; n = n->next) {
			t = (sThread*)n->data;
			ucycles[p->pid] += t->stats.ucycleCount.val64;
			kcycles[p->pid] += t->stats.kcycleCount.val64;
		}
	}
}

void proc_dbg_stopProf(void) {
	sSLNode *n;
	sThread *t;
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		assert(p->pid < PROF_PROC_COUNT);
		uLongLong curUcycles;
		uLongLong curKcycles;
		curUcycles.val64 = 0;
		curKcycles.val64 = 0;
		for(n = sll_begin(p->threads); n != NULL; n = n->next) {
			t = (sThread*)n->data;
			curUcycles.val64 += t->stats.ucycleCount.val64;
			curKcycles.val64 += t->stats.kcycleCount.val64;
		}
		curUcycles.val64 -= ucycles[p->pid];
		curKcycles.val64 -= kcycles[p->pid];
		if(curUcycles.val64 > 0 || curKcycles.val64 > 0) {
			vid_printf("Process %3d (%18s): u=%08x%08x k=%08x%08x\n",
				p->pid,p->command,
				curUcycles.val32.upper,curUcycles.val32.lower,
				curKcycles.val32.upper,curKcycles.val32.lower);
		}
	}
}

void proc_dbg_printAll(void) {
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); )
		proc_dbg_print(p);
}

void proc_dbg_printAllPDs(u8 parts,bool regions) {
	sProc *p;
	for(hm_begin(procs); (p = hm_next(procs)); ) {
		vid_printf("Process %d (%s) (%d own, %d sh, %d sw):\n",
				p->pid,p->command,p->ownFrames,p->sharedFrames,p->swapped);
		if(regions)
			vmm_dbg_print(p);
		paging_dbg_printPDir(p->pagedir,parts);
		vid_printf("\n");
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
