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
#include <sys/task/lock.h>
#include <sys/task/env.h>
#include <sys/task/event.h>
#include <sys/task/uenv.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/kheap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/cow.h>
#include <sys/mem/sharedmem.h>
#include <sys/intrpt.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/node.h>
#include <sys/vfs/real.h>
#include <sys/util.h>
#include <sys/syscalls.h>
#include <sys/video.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

static void proc_notifyProcDied(tPid parent);
static bool proc_add(sProc *p);
static void proc_remove(sProc *p);

/* we have to put the first one here because we need a process before we can allocate
 * something on the kernel-heap. sounds strange, but the kheap-init stuff uses paging and
 * this wants to have the current process. So, I guess the cleanest way is to simply put
 * the first process in the data-area (we can't free the first one anyway) */
static sProc first;
/* our processes */
static sSLList *procs;
static sProc *pidToProc[MAX_PROC_COUNT];
static tPid nextPid = 1;

void proc_init(void) {
	sThread *t;
	tFrameNo stackFrame;
	size_t i;
	
	/* init the first process */
	sProc *p = &first;

	/* do this first because vfs_createProcess may use the kheap so that the kheap needs paging
	 * and paging refers to the current process's pagedir */
	p->pagedir = paging_getCur();
	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(0,&vfs_info_procReadHandler);
	if(p->threadDir == NULL)
		util_panic("Not enough mem for init process");

	p->pid = 0;
	p->parentPid = 0;
	/* 1 pagedir, 1 page-table for kernel-stack, 1 for kernelstack */
	p->ownFrames = 1 + 1 + 1;
	/* the first process has no text, data and stack */
	p->swapped = 0;
	p->exitState = NULL;
	p->sigRetAddr = 0;
	p->flags = 0;
	p->entryPoint = 0;
	p->fsChans = NULL;
	p->env = NULL;
	p->stats.input = 0;
	p->stats.output = 0;
	strcpy(p->command,"initloader");

	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		p->fileDescs[i] = -1;

	/* create first thread */
	p->threads = sll_create();
	if(p->threads == NULL)
		util_panic("Unable to create initial thread-list");
	if(!sll_append(p->threads,thread_init(p)))
		util_panic("Unable to append the initial thread");

	/* setup kernel-stack for us */
	stackFrame = pmem_allocate();
	/* TODO */
#ifndef __mmix__
	paging_map(KERNEL_STACK,&stackFrame,1,PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
#endif

	/* set kernel-stack for first thread */
	t = (sThread*)sll_get(p->threads,0);
	t->kstackFrame = stackFrame;

	/* add to procs */
	if((procs = sll_create()) == NULL)
		util_panic("Unable to create process-list");
	if(!proc_add(p))
		util_panic("Unable to add init-process");
}

tPid proc_getFreePid(void) {
	size_t count = 0;
	while(count < MAX_PROC_COUNT) {
		/* 0 is always present */
		if(nextPid >= MAX_PROC_COUNT)
			nextPid = 1;
		if(pidToProc[nextPid++] == NULL)
			return nextPid - 1;
		count++;
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
	if(pid >= ARRAY_SIZE(pidToProc))
		return NULL;
	return pidToProc[pid];
}

bool proc_exists(tPid pid) {
	if(pid >= MAX_PROC_COUNT)
		return false;
	return pidToProc[pid] != NULL;
}

size_t proc_getCount(void) {
	return sll_length(procs);
}

tFileNo proc_fdToFile(tFD fd) {
	tFileNo fileNo;
	sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	return fileNo;
}

tFD proc_getFreeFd(void) {
	tFD i;
	sProc *p = proc_getRunning();
	tFileNo *fds = p->fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1)
			return i;
	}

	return ERR_MAX_PROC_FDS;
}

int proc_assocFd(tFD fd,tFileNo fileNo) {
	sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	if(p->fileDescs[fd] != -1)
		return ERR_INVALID_FD;

	p->fileDescs[fd] = fileNo;
	return 0;
}

tFD proc_dupFd(tFD fd) {
	tFileNo f;
	tFD nfd;
	int err;
	sProc *p = proc_getRunning();
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	f = p->fileDescs[fd];
	if(f == -1)
		return ERR_INVALID_FD;

	nfd = proc_getFreeFd();
	if(nfd < 0)
		return nfd;

	/* increase references */
	if((err = vfs_incRefs(f)) < 0)
		return err;

	p->fileDescs[nfd] = f;
	return nfd;
}

int proc_redirFd(tFD src,tFD dst) {
	tFileNo fSrc,fDst;
	int err;
	sProc *p = proc_getRunning();

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc == -1 || fDst == -1)
		return ERR_INVALID_FD;

	if((err = vfs_incRefs(fDst)) < 0)
		return err;

	/* we have to close the source because no one else will do it anymore... */
	vfs_closeFile(p->pid,fSrc);

	/* now redirect src to dst */
	p->fileDescs[src] = fDst;
	return 0;
}

tFileNo proc_unassocFd(tFD fd) {
	tFileNo fileNo;
	sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	p->fileDescs[fd] = -1;
	return fileNo;
}

sProc *proc_getProcWithBin(const sBinDesc *bin,tVMRegNo *rno) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		tVMRegNo res = vmm_hasBinary(p,bin);
		if(res != -1) {
			*rno = res;
			return p;
		}
	}
	return NULL;
}

sRegion *proc_getLRURegion(void) {
	tTime ts = (tTime)ULONG_MAX;
	sRegion *lru = NULL;
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		if(p->pid != DISK_PID) {
			sRegion *reg = vmm_getLRURegion(p);
			if(reg && reg->timestamp < ts) {
				ts = reg->timestamp;
				lru = reg;
			}
		}
	}
	return lru;
}

void proc_getMemUsage(size_t *paging,size_t *dataShared,size_t *dataOwn,size_t *dataReal) {
	size_t pages,pmem = 0,ownMem = 0,shMem = 0;
	float dReal = 0;
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		ownMem += p->ownFrames;
		shMem += p->sharedFrames;
		/* + pagedir, page-table for kstack and kstack */
		pmem += paging_getPTableCount(p->pagedir) + 3;
		dReal += vmm_getMemUsage(p,&pages);
	}
	*paging = pmem * PAGE_SIZE;
	*dataOwn = ownMem * PAGE_SIZE;
	*dataShared = shMem * PAGE_SIZE;
	*dataReal = (size_t)(dReal + cow_getFrmCount()) * PAGE_SIZE;
}

bool proc_hasChild(tPid pid) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		if(p->parentPid == pid)
			return true;
	}
	return false;
}

int proc_clone(tPid newPid,uint8_t flags) {
	assert((flags & P_ZOMBIE) == 0);
	tFrameNo stackFrame,dummy;
	size_t i;
	sProc *p;
	sProc *cur = proc_getRunning();
	sThread *curThread = thread_getRunning();
	sThread *nt;
	int res = 0;

	p = (sProc*)kheap_alloc(sizeof(sProc));
	if(!p)
		return ERR_NOT_ENOUGH_MEM;
	/* first create the VFS node (we may not have enough mem) */
	p->threadDir = vfs_createProcess(newPid,&vfs_info_procReadHandler);
	if(p->threadDir == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorProc;
	}

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
	p->entryPoint = cur->entryPoint;
	p->fsChans = NULL;
	p->env = NULL;
	p->stats.input = 0;
	p->stats.output = 0;
	p->flags = flags;
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
	if(p->threads == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorShm;
	}

	/* clone current thread */
	if((res = thread_clone(curThread,&nt,p,&dummy,true)) < 0)
		goto errorThreadList;
	if(!sll_append(p->threads,nt)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorThread;
	}
	/* set kernel-stack-frame; thread_clone() doesn't do it for us */
	nt->kstackFrame = stackFrame;
	/* add to processes */
	if(!proc_add(p)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorThread;
	}

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != -1)
			vfs_incRefs(p->fileDescs[i]);
	}

	/* make thread ready */
	thread_setReady(nt->tid);

#ifdef __eco32__
	debugf("Thread %d (proc %d:%s): %x\n",nt->tid,nt->proc->pid,nt->proc->command,nt->kstackFrame);
#endif
#ifdef __mmix__
	debugf("Thread %d (proc %d:%s)\n",nt->tid,nt->proc->pid,nt->proc->command);
#endif

	res = thread_finishClone(curThread,nt);
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
	return res;
}

int proc_startThread(uintptr_t entryPoint,const void *arg) {
	tFrameNo stackFrame;
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	sThread *nt;
	int res;
	if((res = thread_clone(t,&nt,t->proc,&stackFrame,false)) < 0)
		return res;

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		thread_kill(nt);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* mark ready (idle is always blocked because we choose it explicitly when no other can run) */
	if(nt->tid == IDLE_TID)
		thread_setBlocked(IDLE_TID);
	else
		thread_setReady(nt->tid);

#ifdef __eco32__
	debugf("Thread %d (proc %d:%s): %x\n",nt->tid,nt->proc->pid,nt->proc->command,nt->kstackFrame);
#endif
#ifdef __mmix__
	debugf("Thread %d (proc %d:%s)\n",nt->tid,nt->proc->pid,nt->proc->command);
#endif

	res = thread_finishClone(t,nt);
	if(res == 1) {
		if(!uenv_setupThread(arg,entryPoint)) {
			thread_kill(nt);
			/* do a switch here because we can't continue */
			thread_switch();
			/* never reached */
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	return nt->tid;
}

void proc_destroyThread(int exitCode) {
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
	size_t i;
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
		if(remStack) {
			for(i = 0; i < STACK_REG_COUNT; i++)
				t->stackRegions[i] = -1;
		}
		/* remove all signal-handler since we've removed the code to handle signals */
		sig_removeHandlerFor(t->tid);
	}
}

int proc_getExitState(tPid ppid,sExitState *state) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
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

void proc_terminate(sProc *p,int exitCode,tSig signal) {
	sSLNode *tn,*tmpn;
	size_t i;
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
		p->exitState->schedCount = 0;
		p->exitState->syscalls = 0;
		for(tn = sll_begin(p->threads); tn != NULL; tn = tn->next) {
			sThread *t = (sThread*)tn->data;
			p->exitState->ucycleCount.val64 += t->stats.ucycleCount.val64;
			p->exitState->kcycleCount.val64 += t->stats.kcycleCount.val64;
			p->exitState->schedCount += t->stats.schedCount;
			p->exitState->syscalls += t->stats.syscalls;
		}
		p->exitState->ownFrames = p->ownFrames;
		p->exitState->sharedFrames = p->sharedFrames;
		p->exitState->swapped = p->swapped;
	}

	/* release file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vfs_closeFile(p->pid,p->fileDescs[i]);
			p->fileDescs[i] = -1;
		}
	}

	/* remove all threads */
	for(tn = sll_begin(p->threads); tn != NULL; ) {
		sThread *t = (sThread*)tn->data;
		tmpn = tn->next;
		thread_kill(t);
		tn = tmpn;
	}

	/* remove other stuff */
	env_removeFor(p->pid);
	proc_removeRegions(p,true);
	vfs_real_removeProc(p->pid);
	lock_releaseAll(p->pid);
	if(p->ioMap != NULL) {
		kheap_free(p->ioMap);
		p->ioMap = NULL;
	}

	p->flags |= P_ZOMBIE;
	proc_notifyProcDied(p->parentPid);
}

void proc_kill(sProc *p) {
	sSLNode *n;
	sProc *cur = proc_getRunning();
	vassert(p->pid != 0,"You can't kill the initial process");
	vassert(p != cur,"We can't kill the current process!");

	/* give childs the ppid 0 */
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *cp = (sProc*)n->data;
		if(cp->parentPid == p->pid) {
			cp->parentPid = 0;
			/* if this process has already died, the parent can't wait for it since its dying
			 * right now. therefore notify init of it */
			if(cp->flags & P_ZOMBIE)
				proc_notifyProcDied(0);
		}
	}

	/* remove pagedir; TODO we can do that on terminate, too (if we delay it when its the current) */
	paging_destroyPDir(p->pagedir);
	/* destroy threads-list */
	sll_destroy(p->threads,false);

	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* notify processes that wait for dying procs */
	/* TODO sig_addSignal(SIG_PROC_DIED,p->pid);*/

	/* free exit-state, if not already done */
	if(p->exitState)
		kheap_free(p->exitState);

	/* remove and free */
	proc_remove(p);
	kheap_free(p);
}

static void proc_notifyProcDied(tPid parent) {
	sig_addSignalFor(parent,SIG_CHILD_TERM);
	ev_wakeup(EVI_CHILD_DIED,(tEvObj)proc_getByPid(parent));
}

int proc_buildArgs(const char *const *args,char **argBuffer,size_t *size,bool fromUser) {
	const char *const *arg;
	char *bufPos;
	int argc = 0;
	size_t remaining = EXEC_MAX_ARGSIZE;
	ssize_t len;

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
		if(fromUser && !paging_isRangeUserReadable((uintptr_t)arg,sizeof(char*))) {
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

static bool proc_add(sProc *p) {
	if(!sll_append(procs,p))
		return false;
	pidToProc[p->pid] = p;
	return true;
}

static void proc_remove(sProc *p) {
	sll_removeFirst(procs,p);
	pidToProc[p->pid] = NULL;
}


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

#define PROF_PROC_COUNT		128

static uint64_t ucycles[PROF_PROC_COUNT];
static uint64_t kcycles[PROF_PROC_COUNT];

void proc_dbg_startProf(void) {
	sSLNode *n,*m;
	sThread *t;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		assert(p->pid < PROF_PROC_COUNT);
		ucycles[p->pid] = 0;
		kcycles[p->pid] = 0;
		for(m = sll_begin(p->threads); m != NULL; m = m->next) {
			t = (sThread*)m->data;
			ucycles[p->pid] += t->stats.ucycleCount.val64;
			kcycles[p->pid] += t->stats.kcycleCount.val64;
		}
	}
}

void proc_dbg_stopProf(void) {
	sSLNode *n,*m;
	sThread *t;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		uLongLong curUcycles;
		uLongLong curKcycles;
		assert(p->pid < PROF_PROC_COUNT);
		curUcycles.val64 = 0;
		curKcycles.val64 = 0;
		for(m = sll_begin(p->threads); m != NULL; m = m->next) {
			t = (sThread*)m->data;
			curUcycles.val64 += t->stats.ucycleCount.val64;
			curKcycles.val64 += t->stats.kcycleCount.val64;
		}
		curUcycles.val64 -= ucycles[p->pid];
		curKcycles.val64 -= kcycles[p->pid];
		if(curUcycles.val64 > 0 || curKcycles.val64 > 0) {
			vid_printf("Process %3d (%18s): u=%016Lx k=%016Lx\n",
				p->pid,p->command,curUcycles.val64,curKcycles.val64);
		}
	}
}

void proc_dbg_printAll(void) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		proc_dbg_print(p);
	}
}

void proc_dbg_printAllRegions(void) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vmm_dbg_print(p);
		vid_printf("\n");
	}
}

void proc_dbg_printAllPDs(uint parts,bool regions) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vid_printf("Process %d (%s) (%ld own, %ld sh, %ld sw):\n",
				p->pid,p->command,p->ownFrames,p->sharedFrames,p->swapped);
		if(regions)
			vmm_dbg_print(p);
		paging_dbg_printPDir(p->pagedir,parts);
		vid_printf("\n");
	}
}

void proc_dbg_print(const sProc *p) {
	size_t i;
	sSLNode *n;
	vid_printf("Proc %d:\n",p->pid);
	vid_printf("\tppid=%d, cmd=%s, pdir=%#x, entry=%#Px\n",
			p->parentPid,p->command,p->pagedir,p->entryPoint);
	vid_printf("\tOwnFrames=%lu, sharedFrames=%lu, swapped=%lu\n",
			p->ownFrames,p->sharedFrames,p->swapped);
	if(p->flags & P_ZOMBIE) {
		vid_printf("\tExitstate: code=%d, signal=%d\n",p->exitState->exitCode,p->exitState->signal);
		vid_printf("\t\town=%lu, shared=%lu, swap=%lu\n",
				p->exitState->ownFrames,p->exitState->sharedFrames,p->exitState->swapped);
		vid_printf("\t\tucycles=%#016Lx, kcycles=%#016Lx\n",
				p->exitState->ucycleCount,p->exitState->kcycleCount);
	}
	vid_printf("\tRegions:\n");
	vmm_dbg_printShort(p);
	vid_printf("\tEnvironment:\n");
	env_dbg_printAllOf(p->pid);
	vid_printf("\tFileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			tInodeNo ino;
			tDevNo dev;
			if(vfs_getFileId(p->fileDescs[i],&ino,&dev) == 0) {
				vid_printf("\t\t%d : %d",i,p->fileDescs[i]);
				if(dev == VFS_DEV_NO && vfs_node_isValid(ino))
					vid_printf(" (%s)",vfs_node_getPath(ino));
				vid_printf("\n");
			}
		}
	}
	if(p->threads) {
		for(n = sll_begin(p->threads); n != NULL; n = n->next)
			thread_dbg_print((sThread*)n->data);
	}
	vid_printf("\n");
}

#endif
