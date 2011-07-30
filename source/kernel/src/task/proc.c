/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <sys/task/groups.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/cache.h>
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
#include <sys/debug.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errors.h>

/* the max. size we'll allow for exec()-arguments */
#define EXEC_MAX_ARGSIZE				(2 * K)

static void proc_killThread(sThread *t);
static void proc_notifyProcDied(pid_t parent);
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
static pid_t nextPid = 1;
static sThread *deadThread;

void proc_init(void) {
	size_t i;
	
	/* init the first process */
	sProc *p = &first;

	*(pid_t*)&p->pid = 0;
	paging_setFirst(&p->pagedir);
	p->parentPid = 0;
	p->ruid = ROOT_UID;
	p->euid = ROOT_UID;
	p->suid = ROOT_UID;
	p->rgid = ROOT_GID;
	p->egid = ROOT_GID;
	p->sgid = ROOT_GID;
	p->groups = NULL;
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
	memclear(p->locks,sizeof(p->locks));
	p->command = strdup("initloader");
	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(p->pid,&vfs_info_procReadHandler);
	if(p->threadDir == NULL)
		util_panic("Not enough mem for init process");

	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		p->fileDescs[i] = -1;

	/* create first thread */
	p->threads = sll_create();
	if(p->threads == NULL)
		util_panic("Unable to create initial thread-list");
	if(!sll_append(p->threads,thread_init(p)))
		util_panic("Unable to append the initial thread");

	/* add to procs */
	if((procs = sll_create()) == NULL)
		util_panic("Unable to create process-list");
	if(!proc_add(p))
		util_panic("Unable to add init-process");
}

void proc_setCommand(sProc *p,const char *cmd) {
	if(p->command)
		cache_free((char*)p->command);
	p->command = strdup(cmd);
}

pid_t proc_getFreePid(void) {
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
	const sThread *t = thread_getRunning();
	if(t)
		return t->proc;
	/* just needed at the beginning */
	return &first;
}

sProc *proc_getByPid(pid_t pid) {
	if(pid >= ARRAY_SIZE(pidToProc))
		return NULL;
	return pidToProc[pid];
}

bool proc_exists(pid_t pid) {
	if(pid >= MAX_PROC_COUNT)
		return false;
	return pidToProc[pid] != NULL;
}

size_t proc_getCount(void) {
	return sll_length(procs);
}

file_t proc_fdToFile(int fd) {
	file_t fileNo;
	const sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	return fileNo;
}

int proc_getFreeFd(void) {
	int i;
	const sProc *p = proc_getRunning();
	const file_t *fds = p->fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1)
			return i;
	}

	return ERR_MAX_PROC_FDS;
}

int proc_assocFd(int fd,file_t fileNo) {
	sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	if(p->fileDescs[fd] != -1)
		return ERR_INVALID_FD;

	p->fileDescs[fd] = fileNo;
	return 0;
}

int proc_dupFd(int fd) {
	file_t f;
	int nfd;
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

int proc_redirFd(int src,int dst) {
	file_t fSrc,fDst;
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

file_t proc_unassocFd(int fd) {
	file_t fileNo;
	sProc *p = proc_getRunning();
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	p->fileDescs[fd] = -1;
	return fileNo;
}

sProc *proc_getProcWithBin(const sBinDesc *bin,vmreg_t *rno) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vmreg_t res = vmm_hasBinary(p,bin);
		if(res != -1) {
			*rno = res;
			return p;
		}
	}
	return NULL;
}

sRegion *proc_getLRURegion(void) {
	time_t ts = (time_t)ULONG_MAX;
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
		pmem += paging_getPTableCount(&p->pagedir) + 3;
		dReal += vmm_getMemUsage(p,&pages);
	}
	*paging = pmem * PAGE_SIZE;
	*dataOwn = ownMem * PAGE_SIZE;
	*dataShared = shMem * PAGE_SIZE;
	*dataReal = (size_t)(dReal + cow_getFrmCount()) * PAGE_SIZE;
}

bool proc_hasChild(pid_t pid) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		const sProc *p = (const sProc*)n->data;
		if(p->parentPid == pid)
			return true;
	}
	return false;
}

int proc_clone(pid_t newPid,uint8_t flags) {
	assert((flags & P_ZOMBIE) == 0);
	frameno_t stackFrame;
	size_t i;
	sProc *p;
	const sProc *cur = proc_getRunning();
	sThread *curThread = thread_getRunning();
	sThread *nt;
	int res = 0;

	p = (sProc*)cache_alloc(sizeof(sProc));
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
	*(pid_t*)&p->pid = newPid;
	p->parentPid = cur->pid;
	memclear(p->locks,sizeof(p->locks));
	p->ruid = cur->ruid;
	p->euid = cur->euid;
	p->suid = cur->suid;
	p->rgid = cur->rgid;
	p->egid = cur->egid;
	p->sgid = cur->sgid;
	p->groups = NULL;
	groups_join(p,cur);
	p->exitState = NULL;
	p->sigRetAddr = cur->sigRetAddr;
	p->flags = 0;
	p->entryPoint = cur->entryPoint;
	p->fsChans = NULL;
	p->env = NULL;
	p->stats.input = 0;
	p->stats.output = 0;
	p->flags = flags;
	/* give the process the same name (may be changed by exec) */
	p->command = strdup(cur->command);

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
	if((res = thread_clone(curThread,&nt,p,0,stackFrame,true)) < 0)
		goto errorThreadList;
	if(!sll_append(p->threads,nt)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorThread;
	}
	/* add to processes */
	if(!proc_add(p)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorThread;
	}

	if((res = proc_cloneArch(p,cur) < 0))
		goto errorAdd;

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != -1)
			vfs_incRefs(p->fileDescs[i]);
	}

	/* make thread ready */
	ev_unblock(nt);

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

errorAdd:
	proc_remove(p);
errorThread:
	proc_killThread(nt);
errorThreadList:
	cache_free(p->threads);
errorShm:
	shm_remProc(p);
errorRegs:
	proc_removeRegions(p,true);
errorPDir:
	paging_destroyPDir(&p->pagedir);
errorVFS:
	vfs_removeProcess(newPid);
errorProc:
	cache_free(p);
	return res;
}

int proc_startThread(uintptr_t entryPoint,uint8_t flags,const void *arg) {
	sProc *p = proc_getRunning();
	sThread *t = thread_getRunning();
	sThread *nt;
	int res;
	if((res = thread_clone(t,&nt,t->proc,flags,0,false)) < 0)
		return res;

	/* for the kernel-stack */
	p->ownFrames++;

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		proc_killThread(nt);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* mark ready (idle is always blocked because we choose it explicitly when no other can run) */
	if(nt->flags & T_IDLE)
		ev_block(nt);
	else
		ev_unblock(nt);

#ifdef __eco32__
	debugf("Thread %d (proc %d:%s): %x\n",nt->tid,nt->proc->pid,nt->proc->command,nt->kstackFrame);
#endif
#ifdef __mmix__
	debugf("Thread %d (proc %d:%s)\n",nt->tid,nt->proc->pid,nt->proc->command);
#endif

	res = thread_finishClone(t,nt);
	if(res == 1) {
		if(!uenv_setupThread(arg,entryPoint)) {
			proc_killThread(nt);
			/* do a switch here because we can't continue */
			thread_switch();
			/* never reached */
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	return nt->tid;
}

void proc_exit(int exitCode) {
	sThread *t = thread_getRunning();
	sProc *p = t->proc;
	if(sll_length(p->threads) == 1)
		proc_terminate(p,exitCode,SIG_COUNT);
	else
		proc_killThread(t);
}

void proc_removeRegions(sProc *p,bool remStack) {
	sSLNode *n;
	assert(p);
	/* remove from shared-memory; do this first because it will remove the region and simply
	 * assumes that the region still exists. */
	shm_remProc(p);
	vmm_removeAll(p,remStack);
	/* unset TLS-region (and stack-region, if needed) from all threads */
	for(n = sll_begin(p->threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		thread_removeRegions(t,remStack);
	}
}

int proc_getExitState(pid_t ppid,USER sExitState *state) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		if(p->parentPid == ppid) {
			if(p->flags & P_ZOMBIE) {
				if(state) {
					if(p->exitState) {
						/* if the memcpy segfaults, init will get and free the exitstate */
						memcpy(state,p->exitState,sizeof(sExitState));
						cache_free(p->exitState);
						p->exitState = NULL;
					}
				}
				return p->pid;
			}
		}
	}
	return ERR_NO_CHILD;
}

void proc_segFault(const sProc *p) {
	sThread *t = thread_getRunning();
	sig_addSignalFor(p->pid,SIG_SEGFAULT);
	if(p->flags & P_ZOMBIE)
		thread_switch();
	/* make sure that next time this exception occurs, the process is killed immediatly. otherwise
	 * we might get in an endless-loop */
	sig_unsetHandler(t->tid,SIG_SEGFAULT);
}

void proc_killDeadThread(void) {
	if(deadThread) {
		proc_killThread(deadThread);
		deadThread = NULL;
	}
}

void proc_terminate(sProc *p,int exitCode,sig_t signal) {
	sSLNode *tn,*tmpn;
	size_t i;
	vassert(p->pid != 0,"You can't terminate the initial process");

	/* if its already a zombie and we don't want to kill ourself, kill the process */
	if((p->flags & P_ZOMBIE) && p != proc_getRunning()) {
		proc_kill(p);
		return;
	}

	/* print information to log */
	if(signal != SIG_COUNT) {
		vid_printf("Process %d:%s terminated by signal %d\n",p->pid,p->command,signal);
#if DEBUGGING
		proc_print(p);
#endif
	}

	/* store exit-conditions */
	p->exitState = (sExitState*)cache_alloc(sizeof(sExitState));
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

	/* remove other stuff */
	groups_leave(p);
	env_removeFor(p);
	proc_removeRegions(p,true);
	vfs_real_removeProc(p->pid);
	lock_releaseAll(p->pid);
	proc_terminateArch(p);

	/* remove all threads */
	for(tn = sll_begin(p->threads); tn != NULL; ) {
		sThread *t = (sThread*)tn->data;
		tmpn = tn->next;
		proc_killThread(t);
		tn = tmpn;
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
	paging_destroyPDir(&p->pagedir);
	sll_destroy(p->threads,false);
	cache_free((char*)p->command);
	/* remove from VFS */
	vfs_removeProcess(p->pid);
	/* notify processes that wait for dying procs */
	/* TODO sig_addSignal(SIG_PROC_DIED,p->pid);*/

	/* free exit-state, if not already done */
	if(p->exitState)
		cache_free(p->exitState);

	/* remove and free */
	proc_remove(p);
	cache_free(p);
}

void proc_printAll(void) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		proc_print(p);
	}
}

void proc_printAllRegions(void) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vmm_print(p);
		vid_printf("\n");
	}
}

void proc_printAllPDs(uint parts,bool regions) {
	sSLNode *n;
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vid_printf("Process %d (%s) (%ld own, %ld sh, %ld sw):\n",
				p->pid,p->command,p->ownFrames,p->sharedFrames,p->swapped);
		if(regions)
			vmm_print(p);
		paging_printPDir(&p->pagedir,parts);
		vid_printf("\n");
	}
}

void proc_print(sProc *p) {
	size_t i;
	sSLNode *n;
	vid_printf("Proc %d:\n",p->pid);
	vid_printf("\tppid=%d, cmd=%s, entry=%#Px\n",p->parentPid,p->command,p->entryPoint);
	vid_printf("\tOwner: ruid=%u, euid=%u, suid=%u\n",p->ruid,p->euid,p->suid);
	vid_printf("\tGroup: rgid=%u, egid=%u, sgid=%u\n",p->rgid,p->egid,p->sgid);
	vid_printf("\tGroups: ");
	groups_print(p);
	vid_printf("\n");
	vid_printf("\tFrames: own=%lu, shared=%lu, swapped=%lu\n",
			p->ownFrames,p->sharedFrames,p->swapped);
	if(p->flags & P_ZOMBIE) {
		vid_printf("\tExitstate: code=%d, signal=%d\n",p->exitState->exitCode,p->exitState->signal);
		vid_printf("\t\town=%lu, shared=%lu, swap=%lu\n",
				p->exitState->ownFrames,p->exitState->sharedFrames,p->exitState->swapped);
		vid_printf("\t\tucycles=%#016Lx, kcycles=%#016Lx\n",
				p->exitState->ucycleCount,p->exitState->kcycleCount);
	}
	vid_printf("\tRegions:\n");
	vmm_printShort(p);
	vid_printf("\tEnvironment:\n");
	env_printAllOf(p);
	vid_printf("\tFileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			inode_t ino;
			dev_t dev;
			if(vfs_getFileId(p->fileDescs[i],&ino,&dev) == 0) {
				vid_printf("\t\t%d : %d",i,p->fileDescs[i]);
				if(dev == VFS_DEV_NO && vfs_node_isValid(ino))
					vid_printf(" (%s)",vfs_node_getPath(ino));
				vid_printf("\n");
			}
		}
	}
	vid_printf("\tFS-Channels:\n");
	vfs_real_printFSChans(p);
	if(p->threads) {
		for(n = sll_begin(p->threads); n != NULL; n = n->next)
			thread_print((sThread*)n->data);
	}
	vid_printf("\n");
}

int proc_buildArgs(USER const char *const *args,char **argBuffer,size_t *size,bool fromUser) {
	const char *const *arg;
	char *bufPos;
	int argc = 0;
	size_t remaining = EXEC_MAX_ARGSIZE;
	size_t len;

	/* alloc space for the arguments */
	*argBuffer = (char*)cache_alloc(EXEC_MAX_ARGSIZE);
	if(*argBuffer == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* count args and copy them on the kernel-heap */
	/* note that we have to create a copy since we don't know where the args are. Maybe
	 * they are on the user-stack at the position we want to copy them for the
	 * new process... */
	thread_addHeapAlloc(*argBuffer);
	bufPos = *argBuffer;
	arg = args;
	while(1) {
		/* check if it is a valid pointer */
		if(fromUser && !paging_isInUserSpace((uintptr_t)arg,sizeof(char*)))
			goto error;
		/* end of list? */
		if(*arg == NULL)
			break;

		/* check whether the string is readable */
		if(fromUser && !sysc_isStrInUserSpace(*arg,&len))
			goto error;
		else
			len = strlen(*arg);
		/* ensure that the argument is not longer than the left space */
		if(len >= remaining)
			goto error;

		/* copy to heap */
		memcpy(bufPos,*arg,len + 1);
		bufPos += len + 1;
		remaining -= len + 1;
		arg++;
		argc++;
	}
	thread_remHeapAlloc(*argBuffer);
	/* store args-size and return argc */
	*size = EXEC_MAX_ARGSIZE - remaining;
	return argc;

error:
	thread_remHeapAlloc(*argBuffer);
	cache_free(*argBuffer);
	return ERR_INVALID_ARGS;
}

static void proc_notifyProcDied(pid_t parent) {
	sig_addSignalFor(parent,SIG_CHILD_TERM);
	ev_wakeup(EVI_CHILD_DIED,(evobj_t)proc_getByPid(parent));
}

static void proc_killThread(sThread *t) {
	sProc *p = t->proc;
	if(thread_kill(t)) {
		p->ownFrames--;
		sll_removeFirstWith(p->threads,t);
	}
	else
		deadThread = t;
}

static bool proc_add(sProc *p) {
	if(!sll_append(procs,p))
		return false;
	pidToProc[p->pid] = p;
	return true;
}

static void proc_remove(sProc *p) {
	sll_removeFirstWith(procs,p);
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
		const sProc *p = (const sProc*)n->data;
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
		const sProc *p = (const sProc*)n->data;
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

#endif
