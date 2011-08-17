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
#include <sys/mem/sllnodes.h>
#include <sys/intrpt.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/node.h>
#include <sys/vfs/real.h>
#include <sys/klock.h>
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

static void proc_doRemoveRegions(sProc *p,bool remStack);
static void proc_doTerminate(sProc *p,int exitCode,sig_t signal);
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
static sSLList deadThreads;
static klock_t lock;

void proc_preinit(void) {
	paging_setFirst(&first.pagedir);
}

void proc_init(void) {
	size_t i;
	
	sll_init(&deadThreads,slln_allocNode,slln_freeNode);

	/* init the first process */
	sProc *p = &first;

	*(pid_t*)&p->pid = 0;
	p->parentPid = 0;
	p->ruid = ROOT_UID;
	p->euid = ROOT_UID;
	p->suid = ROOT_UID;
	p->rgid = ROOT_GID;
	p->egid = ROOT_GID;
	p->sgid = ROOT_GID;
	p->groups = NULL;
	p->ownFrames = 0;
	p->sharedFrames = 0;
	p->swapped = 0;
	p->exitState = NULL;
	p->sigRetAddr = 0;
	p->flags = 0;
	p->entryPoint = 0;
	sll_init(&p->fsChans,slln_allocNode,slln_freeNode);
	p->env = NULL;
	p->stats.input = 0;
	p->stats.output = 0;
	memclear(p->locks,sizeof(p->locks));
	p->command = strdup("initloader");
	/* create nodes in vfs */
	p->threadDir = vfs_createProcess(p->pid);
	if(p->threadDir < 0)
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

tPageDir *proc_getPageDir(void) {
	const sThread *t = thread_getRunning();
	/* just needed at the beginning */
	if(t == NULL)
		return &first.pagedir;
	return &t->proc->pagedir;
}

void proc_setCommand(sProc *p,const char *cmd) {
	if(p->command)
		cache_free((char*)p->command);
	p->command = strdup(cmd);
}

pid_t proc_getFreePid(void) {
	size_t count = 0;
	pid_t res = INVALID_PID;
	klock_aquire(&lock);
	while(count < MAX_PROC_COUNT) {
		/* 0 is always present */
		if(nextPid >= MAX_PROC_COUNT)
			nextPid = 1;
		if(pidToProc[nextPid++] == NULL) {
			res = nextPid - 1;
			break;
		}
		count++;
	}
	klock_release(&lock);
	return res;
}

pid_t proc_getRunning(void) {
	const sThread *t = thread_getRunning();
	return t->proc->pid;
}

sProc *proc_getByPid(pid_t pid) {
	if(pid >= ARRAY_SIZE(pidToProc))
		return NULL;
	return pidToProc[pid];
}

sProc *proc_request(pid_t pid,size_t l) {
	sProc *p = proc_getByPid(pid);
	klock_aquire(p->locks + l);
	return p;
}

void proc_release(sProc *p,size_t l) {
	klock_release(p->locks + l);
}

size_t proc_getCount(void) {
	size_t res;
	klock_aquire(&lock);
	res = sll_length(procs);
	klock_release(&lock);
	return res;
}

file_t proc_reqFile(int fd) {
	file_t fileNo;
	sProc *p;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	p = proc_request(proc_getRunning(),PLOCK_FDS);
	fileNo = p->fileDescs[fd];
	if(fileNo == -1)
		fileNo = ERR_INVALID_FD;
	else {
		vfs_incUsages(fileNo);
		thread_addFileUsage(fileNo);
	}
	proc_release(p,PLOCK_FDS);
	return fileNo;
}

void proc_relFile(file_t file) {
	thread_remFileUsage(file);
	vfs_decUsages(file);
}

int proc_assocFd(file_t fileNo) {
	sProc *p = proc_request(proc_getRunning(),PLOCK_FDS);
	const file_t *fds = p->fileDescs;
	int i,fd = ERR_MAX_PROC_FDS;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1) {
			fd = i;
			break;
		}
	}
	if(fd >= 0)
		p->fileDescs[fd] = fileNo;
	proc_release(p,PLOCK_FDS);
	return fd;
}

int proc_dupFd(int fd) {
	file_t f;
	int i,nfd = ERR_INVALID_FD;
	sProc *p = proc_request(proc_getRunning(),PLOCK_FDS);
	const file_t *fds = p->fileDescs;
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT) {
		proc_release(p,PLOCK_FDS);
		return ERR_INVALID_FD;
	}

	f = p->fileDescs[fd];
	if(f >= 0) {
		nfd = ERR_MAX_PROC_FDS;
		for(i = 0; i < MAX_FD_COUNT; i++) {
			if(fds[i] == -1) {
				/* increase references */
				nfd = i;
				vfs_incRefs(f);
				p->fileDescs[nfd] = f;
				break;
			}
		}
	}
	proc_release(p,PLOCK_FDS);
	return nfd;
}

int proc_redirFd(int src,int dst) {
	file_t fSrc,fDst;
	int err = ERR_INVALID_FD;
	sProc *p;

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	p = proc_request(proc_getRunning(),PLOCK_FDS);
	fSrc = p->fileDescs[src];
	fDst = p->fileDescs[dst];
	if(fSrc >= 0 && fDst >= 0) {
		vfs_incRefs(fDst);
		/* we have to close the source because no one else will do it anymore... */
		vfs_closeFile(p->pid,fSrc);
		/* now redirect src to dst */
		p->fileDescs[src] = fDst;
		err = 0;
	}
	proc_release(p,PLOCK_FDS);
	return err;
}

file_t proc_unassocFd(int fd) {
	file_t fileNo;
	sProc *p;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	p = proc_request(proc_getRunning(),PLOCK_FDS);
	fileNo = p->fileDescs[fd];
	if(fileNo >= 0)
		p->fileDescs[fd] = -1;
	else
		fileNo = ERR_INVALID_FD;
	proc_release(p,PLOCK_FDS);
	return fileNo;
}

pid_t proc_getProcWithBin(const sBinDesc *bin,vmreg_t *rno) {
	sSLNode *n;
	pid_t res = INVALID_PID;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vmreg_t regno = vmm_hasBinary(p->pid,bin);
		if(regno != -1) {
			*rno = regno;
			res = p->pid;
			break;
		}
	}
	klock_release(&lock);
	return res;
}

sRegion *proc_getLRURegion(void) {
	time_t ts = (time_t)ULONG_MAX;
	sRegion *lru = NULL;
	sSLNode *n;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		if(p->pid != DISK_PID) {
			sRegion *reg = vmm_getLRURegion(p->pid);
			if(reg && reg->timestamp < ts) {
				ts = reg->timestamp;
				lru = reg;
			}
		}
	}
	klock_release(&lock);
	return lru;
}

void proc_getMemUsageOf(pid_t pid,size_t *own,size_t *shared,size_t *swapped) {
	sProc *p = proc_getByPid(pid);
	if(p) {
		vmm_getMemUsageOf(pid,own,shared,swapped);
		*own = *own + paging_getPTableCount(&p->pagedir) + proc_getKMemUsageOf(p);
	}
}

void proc_getMemUsage(size_t *paging,size_t *dataShared,size_t *dataOwn,size_t *dataReal) {
	size_t pages,pmem = 0,ownMem = 0,shMem = 0;
	float dReal = 0;
	sSLNode *n;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		size_t pown,psh,pswap;
		sProc *p = (sProc*)n->data;
		proc_getMemUsageOf(p->pid,&pown,&psh,&pswap);
		ownMem += pown;
		shMem += psh;
		dReal += vmm_getMemUsage(p->pid,&pages);
	}
	klock_release(&lock);
	*paging = pmem * PAGE_SIZE;
	*dataOwn = ownMem * PAGE_SIZE;
	*dataShared = shMem * PAGE_SIZE;
	*dataReal = (size_t)(dReal + cow_getFrmCount()) * PAGE_SIZE;
}

void proc_addSignalFor(pid_t pid,sig_t signal) {
	sProc *p = proc_request(pid,PLOCK_PROG);
	if(p) {
		bool sent = false;
		sSLNode *n;
		for(n = sll_begin(p->threads); n != NULL; n = n->next) {
			sThread *t = (sThread*)n->data;
			if(sig_addSignalFor(t->tid,signal)) {
				sent = true;
				break;
			}
		}
		proc_release(p,PLOCK_PROG);
		/* no handler and fatal? terminate proc! */
		if(!sent && sig_isFatal(signal)) {
			proc_terminate(pid,1,signal);
			if(pid == proc_getRunning())
				thread_switch();
		}
	}
}

int proc_clone(pid_t newPid,uint8_t flags) {
	assert((flags & P_ZOMBIE) == 0);
	size_t i;
	sProc *p;
	sProc *cur = proc_request(proc_getRunning(),PLOCK_PROG);
	sThread *curThread = thread_getRunning();
	sThread *nt;
	int res = 0;
	if(!cur)
		return ERR_INVALID_PID;

	p = (sProc*)cache_alloc(sizeof(sProc));
	if(!p) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorCur;
	}
	/* first create the VFS node (we may not have enough mem) */
	p->threadDir = vfs_createProcess(newPid);
	if(p->threadDir < 0) {
		res = p->threadDir;
		goto errorProc;
	}

	/* clone page-dir */
	if((res = paging_cloneKernelspace(&p->pagedir)) < 0)
		goto errorVFS;

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
	p->exitState = NULL;
	p->sigRetAddr = cur->sigRetAddr;
	p->flags = 0;
	p->entryPoint = cur->entryPoint;
	sll_init(&p->fsChans,slln_allocNode,slln_freeNode);
	p->env = NULL;
	p->stats.input = 0;
	p->stats.output = 0;
	p->flags = flags;
	/* give the process the same name (may be changed by exec) */
	p->command = strdup(cur->command);
	if(p->command == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorPdir;
	}

	/* add to processes */
	if(!proc_add(p)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorCmd;
	}

	/* join group of parent */
	p->groups = NULL;
	groups_join(p,cur);

	/* clone regions */
	p->regions = NULL;
	p->regSize = 0;
	if((res = vmm_cloneAll(p->pid)) < 0)
		goto errorAdd;

	/* clone shared-memory-regions */
	if((res = shm_cloneProc(cur->pid,p->pid)) < 0)
		goto errorRegs;

	/* create thread-list */
	p->threads = sll_create();
	if(p->threads == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorShm;
	}

	/* clone current thread */
	if((res = thread_create(curThread,&nt,p,0,true)) < 0)
		goto errorThreadList;
	if(!sll_append(p->threads,nt)) {
		res = ERR_NOT_ENOUGH_MEM;
		goto errorThread;
	}

	if((res = proc_cloneArch(p,cur) < 0))
		goto errorThread;

	/* inherit file-descriptors */
	proc_request(cur->pid,PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		p->fileDescs[i] = cur->fileDescs[i];
		if(p->fileDescs[i] != -1)
			vfs_incRefs(p->fileDescs[i]);
	}
	proc_release(cur,PLOCK_FDS);

	/* make thread ready */
	ev_unblock(nt);

#ifdef __eco32__
	debugf("Thread %d (proc %d:%s): %x\n",nt->tid,nt->proc->pid,nt->proc->command,
			nt->archAttr.kstackFrame);
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
	proc_release(cur,PLOCK_PROG);
	return 0;

errorThread:
	proc_killThread(nt);
errorThreadList:
	cache_free(p->threads);
errorShm:
	shm_remProc(p->pid);
errorRegs:
	proc_doRemoveRegions(p,true);
errorAdd:
	groups_leave(p->pid);
	proc_remove(p);
errorCmd:
	cache_free((void*)p->command);
errorPdir:
	paging_destroyPDir(&p->pagedir);
errorVFS:
	vfs_removeProcess(newPid);
errorProc:
	cache_free(p);
errorCur:
	proc_release(cur,PLOCK_PROG);
	return res;
}

int proc_startThread(uintptr_t entryPoint,uint8_t flags,const void *arg) {
	sProc *p = proc_request(proc_getRunning(),PLOCK_PROG);
	sThread *t = thread_getRunning();
	sThread *nt;
	int res;
	if(!p)
		return ERR_INVALID_PID;

	if((res = thread_create(t,&nt,p,flags,false)) < 0) {
		proc_release(p,PLOCK_PROG);
		return res;
	}

	/* append thread */
	if(!sll_append(p->threads,nt)) {
		proc_killThread(nt);
		proc_release(p,PLOCK_PROG);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* mark ready (idle is always blocked because we choose it explicitly when no other can run) */
	if(nt->flags & T_IDLE)
		ev_block(nt);
	else
		ev_unblock(nt);

#ifdef __eco32__
	debugf("Thread %d (proc %d:%s): %x\n",nt->tid,nt->proc->pid,nt->proc->command,
			nt->archAttr.kstackFrame);
#endif
#ifdef __mmix__
	debugf("Thread %d (proc %d:%s)\n",nt->tid,nt->proc->pid,nt->proc->command);
#endif

	thread_finishThreadStart(t,nt,arg,entryPoint);
	proc_release(p,PLOCK_PROG);
	return nt->tid;
}

int proc_exec(const char *path,const char *const *args,const void *code,size_t size) {
	char *argBuffer;
	sStartupInfo info;
	sProc *p = proc_request(proc_getRunning(),PLOCK_PROG);
	size_t argSize;
	int argc;
	if(!p)
		return ERR_INVALID_PID;
	/* we can't do an exec if we have multiple threads (init can do that, because the threads are
	 * "kernel-threads") */
	if(p->pid != 0 && sll_length(p->threads) > 1) {
		proc_release(p,PLOCK_PROG);
		return ERR_INVALID_ARGS;
	}

	if(!code) {
		/* resolve path; require a path in real fs */
		inode_t nodeNo;
		if(vfs_node_resolvePath(path,&nodeNo,NULL,VFS_READ) != ERR_REAL_PATH) {
			proc_release(p,PLOCK_PROG);
			return ERR_INVALID_ARGS;
		}
	}

	argc = 0;
	argBuffer = NULL;
	if(args != NULL) {
		argc = proc_buildArgs(args,&argBuffer,&argSize,!code);
		if(argc < 0)
			return argc;
	}

	/* remove all except stack */
	proc_doRemoveRegions(p,false);

	/* load program */
	proc_release(p,PLOCK_PROG);
	if(code) {
		if(elf_loadFromMem(code,size,&info) < 0)
			goto errorNoRel;
	}
	else {
		if(elf_loadFromFile(path,&info) < 0)
			goto errorNoRel;
	}
	p = proc_request(p->pid,PLOCK_PROG);

	/* copy path so that we can identify the process */
	proc_setCommand(p,path);

#ifdef __eco32__
	debugf("EXEC: proc %d:%s\n",p->pid,p->command);
#endif
#ifdef __mmix__
	debugf("EXEC: proc %d:%s\n",p->pid,p->command);
#endif

	/* make process ready */
	/* the entry-point is the one of the process, since threads don't start with the dl again */
	p->entryPoint = info.progEntry;
	thread_addHeapAlloc(argBuffer);
	/* for starting use the linker-entry, which will be progEntry if no dl is present */
	if(!uenv_setupProc(path,argc,argBuffer,argSize,&info,info.linkerEntry))
		goto error;
	thread_remHeapAlloc(argBuffer);
	cache_free(argBuffer);
	proc_release(p,PLOCK_PROG);
	return 0;

error:
	proc_release(p,PLOCK_PROG);
errorNoRel:
	cache_free(argBuffer);
	proc_terminate(p->pid,1,SIG_COUNT);
	thread_switch();
	util_panic("We should not reach this!");
	/* not reached */
	return 0;
}

void proc_exit(int exitCode) {
	sThread *t = thread_getRunning();
	sProc *p = proc_request(t->proc->pid,PLOCK_PROG);
	if(sll_length(p->threads) == 1) {
		proc_doTerminate(p,exitCode,SIG_COUNT);
		proc_release(p,PLOCK_PROG);
		proc_notifyProcDied(p->parentPid);
	}
	else {
		proc_killThread(t);
		proc_release(p,PLOCK_PROG);
	}
}

void proc_removeRegions(pid_t pid,bool remStack) {
	sProc *p = proc_request(pid,PLOCK_PROG);
	proc_doRemoveRegions(p,remStack);
	proc_release(p,PLOCK_PROG);
}

static void proc_doRemoveRegions(sProc *p,bool remStack) {
	sSLNode *n;
	/* remove from shared-memory; do this first because it will remove the region and simply
	 * assumes that the region still exists. */
	shm_remProc(p->pid);
	vmm_removeAll(p->pid,remStack);
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
			p = proc_request(p->pid,PLOCK_PROG);
			if(p->flags & P_ZOMBIE) {
				if(state) {
					if(p->exitState) {
						/* if the memcpy segfaults, init will get and free the exitstate */
						memcpy(state,p->exitState,sizeof(sExitState));
						cache_free(p->exitState);
						p->exitState = NULL;
					}
				}
				proc_release(p,PLOCK_PROG);
				return p->pid;
			}
			proc_release(p,PLOCK_PROG);
		}
	}
	return ERR_NO_CHILD;
}

void proc_segFault(void) {
	sThread *t = thread_getRunning();
	proc_addSignalFor(t->proc->pid,SIG_SEGFAULT);
	/* make sure that next time this exception occurs, the process is killed immediatly. otherwise
	 * we might get in an endless-loop */
	sig_unsetHandler(t->tid,SIG_SEGFAULT);
}

void proc_killDeadThread(void) {
	klock_aquire(&lock);
	if(sll_length(&deadThreads) > 0) {
		sSLNode *n;
		for(n = sll_begin(&deadThreads); n != NULL; n = n->next) {
			sThread *t = (sThread*)n->data;
			sProc *p = proc_request(t->proc->pid,PLOCK_PROG);
			thread_kill(t);
			sll_removeFirstWith(t->proc->threads,t);
			proc_release(p,PLOCK_PROG);
		}
		sll_clear(&deadThreads,false);
	}
	klock_release(&lock);
}

void proc_terminate(pid_t pid,int exitCode,sig_t signal) {
	sProc *p = proc_request(pid,PLOCK_PROG);
	/* if its already a zombie and we don't want to kill ourself, kill the process */
	if((p->flags & P_ZOMBIE) && pid != proc_getRunning()) {
		proc_release(p,PLOCK_PROG);
		proc_kill(pid);
		return;
	}
	proc_doTerminate(p,exitCode,signal);
	proc_release(p,PLOCK_PROG);
	proc_notifyProcDied(p->parentPid);
}

static void proc_doTerminate(sProc *p,int exitCode,sig_t signal) {
	sSLNode *tn,*tmpn;
	size_t i;
	vassert(p->pid != 0,"You can't terminate the initial process");

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
	klock_aquire(p->locks + PLOCK_FDS);
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vfs_incUsages(p->fileDescs[i]);
			if(!vfs_closeFile(p->pid,p->fileDescs[i]))
				vfs_decUsages(p->fileDescs[i]);
			p->fileDescs[i] = -1;
		}
	}
	klock_release(p->locks + PLOCK_FDS);

	/* remove other stuff */
	groups_leave(p->pid);
	env_removeFor(p->pid);
	proc_doRemoveRegions(p,true);
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
}

bool proc_kill(pid_t pid) {
	sSLNode *n;
	sProc *p = proc_getByPid(pid);
	if(!p)
		return false;

	vassert(p->pid != 0,"You can't kill the initial process");
	vassert(p->pid != proc_getRunning(),"We can't kill the current process!");

	/* give childs the ppid 0 */
	klock_aquire(&lock);
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
	klock_release(&lock);

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
	return true;
}

void proc_printAll(void) {
	sSLNode *n;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		proc_print(p);
	}
	klock_release(&lock);
}

void proc_printAllRegions(void) {
	sSLNode *n;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		vmm_print(p->pid);
		vid_printf("\n");
	}
	klock_release(&lock);
}

void proc_printAllPDs(uint parts,bool regions) {
	sSLNode *n;
	klock_aquire(&lock);
	for(n = sll_begin(procs); n != NULL; n = n->next) {
		sProc *p = (sProc*)n->data;
		size_t own,shared,swapped;
		proc_getMemUsageOf(p->pid,&own,&shared,&swapped);
		vid_printf("Process %d (%s) (%ld own, %ld sh, %ld sw):\n",
				p->pid,p->command,own,shared,swapped);
		if(regions)
			vmm_print(p->pid);
		paging_printPDir(&p->pagedir,parts);
		vid_printf("\n");
	}
	klock_release(&lock);
}

void proc_print(sProc *p) {
	size_t i,own,shared,swapped;
	sSLNode *n;
	vid_printf("Proc %d:\n",p->pid);
	vid_printf("\tppid=%d, cmd=%s, entry=%#Px\n",p->parentPid,p->command,p->entryPoint);
	vid_printf("\tOwner: ruid=%u, euid=%u, suid=%u\n",p->ruid,p->euid,p->suid);
	vid_printf("\tGroup: rgid=%u, egid=%u, sgid=%u\n",p->rgid,p->egid,p->sgid);
	vid_printf("\tGroups: ");
	groups_print(p->pid);
	vid_printf("\n");
	proc_getMemUsageOf(p->pid,&own,&shared,&swapped);
	vid_printf("\tFrames: own=%lu, shared=%lu, swapped=%lu\n",own,shared,swapped);
	if(p->flags & P_ZOMBIE) {
		vid_printf("\tExitstate: code=%d, signal=%d\n",p->exitState->exitCode,p->exitState->signal);
		vid_printf("\t\town=%lu, shared=%lu, swap=%lu\n",
				p->exitState->ownFrames,p->exitState->sharedFrames,p->exitState->swapped);
		vid_printf("\t\tucycles=%#016Lx, kcycles=%#016Lx\n",
				p->exitState->ucycleCount,p->exitState->kcycleCount);
	}
	vid_printf("\tRegions:\n");
	vmm_printShort(p->pid);
	vid_printf("\tEnvironment:\n");
	env_printAllOf(p->pid);
	vid_printf("\tFileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(p->fileDescs[i] != -1) {
			vid_printf("\t\t%-2d: ",i);
			vfs_printFile(p->fileDescs[i]);
			vid_printf("\n");
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
	proc_addSignalFor(parent,SIG_CHILD_TERM);
	ev_wakeup(EVI_CHILD_DIED,(evobj_t)proc_getByPid(parent));
}

static void proc_killThread(sThread *t) {
	sProc *p = t->proc;
	if(thread_kill(t))
		sll_removeFirstWith(p->threads,t);
	else {
		klock_aquire(&lock);
		assert(sll_append(&deadThreads,t));
		klock_release(&lock);
	}
}

static bool proc_add(sProc *p) {
	bool res = false;
	klock_aquire(&lock);
	if(sll_append(procs,p)) {
		pidToProc[p->pid] = p;
		res = true;
	}
	klock_release(&lock);
	return res;
}

static void proc_remove(sProc *p) {
	klock_aquire(&lock);
	sll_removeFirstWith(procs,p);
	pidToProc[p->pid] = NULL;
	klock_release(&lock);
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
