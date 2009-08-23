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
#include <thread.h>
#include <debug.h>
#include <proc.h>
#include <cpu.h>
#include <gdt.h>
#include <kheap.h>
#include <paging.h>
#include <sched.h>
#include <util.h>
#include <signals.h>
#include <vfs.h>
#include <vfsinfo.h>
#include <vfsnode.h>
#include <timer.h>
#include <kevent.h>
#include <video.h>
#include <sllist.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define THREAD_MAP_SIZE		1024

/* our map for the threads. key is (tid % THREAD_MAP_SIZE) */
static sSLList *threadMap[THREAD_MAP_SIZE] = {NULL};
static sThread *cur = NULL;
static tTid nextTid = 0;

/* list of dead threads that should be destroyed */
static sSLList* deadThreads = NULL;

sThread *thread_init(sProc *p) {
	tFD i;
	sThread *t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		util_panic("Unable to allocate mem for initial thread");

	t->state = ST_RUNNING;
	t->events = EV_NOEVENT;
	t->waitsInKernel = 0;
	t->tid = nextTid++;
	t->proc = p;
	/* we'll give the thread a stack later */
	t->ustackBegin = KERNEL_AREA_V_ADDR;
	t->ustackPages = 0;
	t->ucycleCount.val64 = 0;
	t->ucycleStart = 0;
	t->kcycleCount.val64 = 0;
	t->kcycleStart = 0;
	t->fpuState = NULL;
	t->signal = 0;
	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		t->fileDescs[i] = -1;

	/* create list */
	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE] = sll_create();
	if(list == NULL)
		util_panic("Unable to allocate mem for initial thread-list");

	/* append */
	if(!sll_append(list,t))
		util_panic("Unable to append initial thread");

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid,vfsinfo_threadReadHandler))
		util_panic("Unable to put first thread in vfs");

	cur = t;
	return t;
}

u32 thread_getCount(void) {
	u32 i,count = 0;
	for(i = 0; i < THREAD_MAP_SIZE; i++)
		count += sll_length(threadMap[i]);
	return count;
}

sThread *thread_getRunning(void) {
	return cur;
}

sThread *thread_getById(tTid tid) {
	sSLNode *n;
	sThread *t;
	sSLList *list = threadMap[tid % THREAD_MAP_SIZE];
	if(list == NULL)
		return NULL;
	for(n = sll_begin(list); n != NULL; n = n->next) {
		t = (sThread*)n->data;
		if(t->tid == tid)
			return t;
	}
	return NULL;
}

void thread_switch(void) {
	thread_switchTo(sched_perform()->tid);
}

void thread_switchTo(tTid tid) {
	u64 kcstart = cur->kcycleStart;
	/* finish kernel-time here since we're switching the process */
	if(kcstart > 0) {
		u64 cycles = cpu_rdtsc();
		cur->kcycleCount.val64 += cycles - kcstart;
	}

	if(tid != cur->tid && !thread_save(&cur->save)) {
		sThread *t = thread_getById(tid);
		vassert(t != NULL,"Thread with tid %d not found",tid);
		sThread *old;

		/* mark old process ready, if it should not be blocked, killed or something */
		if(cur->state == ST_RUNNING)
			sched_setReady(cur);

		old = cur;
		cur = t;
		sched_setRunning(cur);

		if(old->proc != cur->proc) {
			/* remove the io-map. it will be set as soon as the process accesses an io-port
			 * (we'll get an exception) */
			tss_removeIOMap();
		}

		/* lock the FPU so that we can save the FPU-state for the previous process as soon
		 * as this one wants to use the FPU */
		fpu_lockFPU();
		thread_resume(cur->proc->physPDirAddr,&cur->save,cur->kstackFrame);
	}

	/* now start kernel-time again */
	cur->kcycleStart = cpu_rdtsc();

	/* destroy processes, if there is any */
	proc_cleanup();
	/* destroy process, if there is any */
	if(deadThreads != NULL) {
		sSLNode *n;
		for(n = sll_begin(deadThreads); n != NULL; n = n->next) {
			/* we want to destroy the stacks here because if the whole process is destroyed
			 * the proc-module doesn't kill the running thread anyway so there will never
			 * be the case that we should destroy a single thread later */
			thread_destroy((sThread*)n->data,true);
		}
		sll_destroy(deadThreads,false);
		deadThreads = NULL;
	}
}

void thread_switchInKernel(void) {
	/* remember that the current thread waits in the kernel */
	/* atm this is just used by the signal-module to check wether we can send a signal to a
	 * thread or not */
	cur->waitsInKernel = 1;
	thread_switch();
	cur->waitsInKernel = 0;
	kev_notify(KEV_KWAIT_DONE,cur->tid);
}

void thread_wait(tTid tid,u8 events) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL,"Thread with id %d not found",tid);
	t->events = events;
	sched_setBlocked(t);
}

void thread_wakeupAll(u8 event) {
	sched_unblockAll(event);
}

void thread_wakeup(tTid tid,u8 event) {
	sThread *t = thread_getById(tid);
	/* ignore the wakeup if the thread doesn't exist */
	if(t == NULL)
		return;
	if(t->events & event) {
		/* TODO somehow it is by far slower to use setReadyQuick() here. I can't really
		 * explain this behaviour :/ */
		/*sched_setReadyQuick(t);*/
		sched_setReady(t);
		t->events = EV_NOEVENT;
	}
}

tFileNo thread_fdToFile(tFD fd) {
	tFileNo fileNo;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = cur->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	return fileNo;
}

tFD thread_getFreeFd(void) {
	tFD i;
	tFileNo *fds = cur->fileDescs;
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(fds[i] == -1)
			return i;
	}

	return ERR_MAX_PROC_FDS;
}

s32 thread_assocFd(tFD fd,tFileNo fileNo) {
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	if(cur->fileDescs[fd] != -1)
		return ERR_INVALID_FD;

	cur->fileDescs[fd] = fileNo;
	return 0;
}

tFD thread_dupFd(tFD fd) {
	tFileNo f;
	s32 err,nfd;
	/* check fd */
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	f = cur->fileDescs[fd];
	if(f == -1)
		return ERR_INVALID_FD;

	nfd = thread_getFreeFd();
	if(nfd < 0)
		return nfd;

	/* increase references */
	if((err = vfs_incRefs(f)) < 0)
		return err;

	cur->fileDescs[nfd] = f;
	return nfd;
}

s32 thread_redirFd(tFD src,tFD dst) {
	tFileNo fSrc,fDst;
	s32 err;

	/* check fds */
	if(src < 0 || src >= MAX_FD_COUNT || dst < 0 || dst >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fSrc = cur->fileDescs[src];
	fDst = cur->fileDescs[dst];
	if(fSrc == -1 || fDst == -1)
		return ERR_INVALID_FD;

	if((err = vfs_incRefs(fDst)) < 0)
		return err;

	/* we have to close the source because no one else will do it anymore... */
	vfs_closeFile(cur->tid,fSrc);

	/* now redirect src to dst */
	cur->fileDescs[src] = fDst;
	return 0;
}

tFileNo thread_unassocFd(tFD fd) {
	tFileNo fileNo;
	if(fd < 0 || fd >= MAX_FD_COUNT)
		return ERR_INVALID_FD;

	fileNo = cur->fileDescs[fd];
	if(fileNo == -1)
		return ERR_INVALID_FD;

	cur->fileDescs[fd] = -1;
	return fileNo;
}

s32 thread_extendStack(u32 address) {
	s32 newPages;
	address &= ~(PAGE_SIZE - 1);
	newPages = ((cur->ustackBegin - address) >> PAGE_SIZE_SHIFT) - cur->ustackPages;
	if(newPages > 0) {
		if(cur->ustackPages + newPages > MAX_STACK_PAGES)
			return ERR_NOT_ENOUGH_MEM;
		if(!proc_segSizesValid(cur->proc->textPages,cur->proc->dataPages,cur->proc->stackPages + newPages))
			return ERR_NOT_ENOUGH_MEM;
		if(!proc_changeSize(newPages,CHG_STACK))
			return ERR_NOT_ENOUGH_MEM;
	}
	return 0;
}

s32 thread_clone(sThread *src,sThread **dst,sProc *p,u32 *stackFrame,bool cloneProc) {
	tFD i;
	sThread *t = *dst;
	t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		return ERR_NOT_ENOUGH_MEM;

	t->tid = nextTid++;
	t->state = ST_RUNNING;
	t->events = src->events;
	t->waitsInKernel = 0;
	fpu_cloneState(&(t->fpuState),src->fpuState);
	t->kcycleCount.val64 = 0;
	t->kcycleStart = 0;
	t->ucycleCount.val64 = 0;
	t->ucycleStart = 0;
	t->proc = p;
	t->signal = 0;
	if(cloneProc) {
		/* proc_clone() sets t->kstackFrame in this case */
		t->ustackBegin = src->ustackBegin;
		t->ustackPages = src->ustackPages;
	}
	else {
		/* find an unused stack-beginning. we don't know which stack-begins are occupied because
		 * if we clone a process we clone just the current thread, which is an arbitrary one. And
		 * the stack is kept at the same place. So we have to search which begin is free...
		 */
		sSLNode *n;
		bool ubeginOk;
		u32 ustackBegin = KERNEL_AREA_V_ADDR;
		do {
			ubeginOk = true;
			for(n = sll_begin(src->proc->threads); n != NULL; n = n->next) {
				sThread *pt = (sThread*)n->data;
				if(ustackBegin == pt->ustackBegin) {
					ubeginOk = false;
					break;
				}
			}
			if(!ubeginOk)
				ustackBegin -= MAX_STACK_PAGES * PAGE_SIZE;
		}
		while(!ubeginOk);

		/* create kernel-stack */
		t->ustackBegin = ustackBegin;
		*stackFrame = t->kstackFrame = mm_allocateFrame(MM_DEF);

		/* initial user-stack-pages */
		t->ustackPages = INITIAL_STACK_PAGES;
		paging_map(t->ustackBegin - t->ustackPages * PAGE_SIZE,NULL,t->ustackPages,PG_WRITABLE,true);
	}

	/* create thread-list if necessary */
	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE];
	if(list == NULL) {
		list = threadMap[t->tid % THREAD_MAP_SIZE] = sll_create();
		if(list == NULL) {
			if(!cloneProc) {
				mm_freeFrame(t->kstackFrame,MM_DEF);
				paging_unmap(t->ustackBegin - t->ustackPages * PAGE_SIZE,t->ustackPages,true,true);
			}
			kheap_free(t);
			return ERR_NOT_ENOUGH_MEM;
		}
	}

	/* insert */
	if(!sll_append(list,t)) {
		if(!cloneProc) {
			mm_freeFrame(t->kstackFrame,MM_DEF);
			paging_unmap(t->ustackBegin - t->ustackPages * PAGE_SIZE,t->ustackPages,true,true);
		}
		kheap_free(t);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid,vfsinfo_threadReadHandler)) {
		if(!cloneProc) {
			mm_freeFrame(t->kstackFrame,MM_DEF);
			paging_unmap(t->ustackBegin - t->ustackPages * PAGE_SIZE,t->ustackPages,true,true);
		}
		kheap_free(t);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		t->fileDescs[i] = src->fileDescs[i];
		if(t->fileDescs[i] != -1)
			t->fileDescs[i] = vfs_inheritFileNo(t->tid,t->fileDescs[i]);
	}

	*dst = t;
	return 0;
}

void thread_destroy(sThread *t,bool destroyStacks) {
	tFD i;
	/* we can't destroy the current thread */
	if(t == cur) {
		/* put it in the dead-thread-queue to destroy it later */
		if(deadThreads == NULL) {
			deadThreads = sll_create();
			if(deadThreads == NULL) {
				kheap_dbg_print();
				util_panic("Not enough mem for dead thread-list");
			}
		}
		if(!sll_append(deadThreads,t))
			util_panic("Not enough mem to append dead thread");
		/* remove from scheduler and ensure that he don't picks us again */
		sched_removeThread(t);
		t->state = ST_ZOMBIE;
		return;
	}

	sSLList *list = threadMap[t->tid % THREAD_MAP_SIZE];
	vassert(list != NULL,"Thread %d not found in thread-map",t->tid);

	/* destroy stacks */
	if(destroyStacks)
		paging_destroyStacks(t);

	/* release file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(t->fileDescs[i] != -1) {
			vfs_closeFile(t->tid,t->fileDescs[i]);
			t->fileDescs[i] = -1;
		}
	}

	/* remove from process */
	t->proc->stackPages -= t->ustackPages;
	sll_removeFirst(t->proc->threads,t);

	/* remove from all modules we may be announced */
	sched_removeThread(t);
	timer_removeThread(t->tid);
	fpu_freeState(&t->fpuState);
	sig_removeHandlerFor(t->tid);
	vfs_removeThread(t->tid);

	/* notify others that wait for dying threads */
	sig_addSignal(SIG_THREAD_DIED,t->tid);

	/* finally, destroy thread */
	sll_removeFirst(list,t);
	kheap_free(t);
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void thread_dbg_printAll(void) {
	sSLList *list;
	sSLNode *n;
	u32 i;
	vid_printf("Threads:\n");
	for(i = 0; i < THREAD_MAP_SIZE; i++) {
		list = threadMap[i];
		if(list) {
			for(n = sll_begin(list); n != NULL; n = n->next) {
				sThread *t = (sThread*)n->data;
				thread_dbg_print(t);
			}
		}
	}
}

void thread_dbg_print(sThread *t) {
	static const char *states[] = {"UNUSED","RUNNING","READY","BLOCKED","ZOMBIE"};
	tFD i;
	vid_printf("\tthread %d: (process %d:%s)\n",t->tid,t->proc->pid,t->proc->command);
	vid_printf("\t\tstate=%s\n",states[t->state]);
	vid_printf("\t\tustack=0x%08x (%d pages)\n",t->ustackBegin,t->ustackPages);
	vid_printf("\t\tkstackFrame=0x%x\n",t->kstackFrame);
	vid_printf("\t\tucycleCount = 0x%08x%08x\n",t->ucycleCount.val32.upper,
			t->ucycleCount.val32.lower);
	vid_printf("\t\tkcycleCount = 0x%08x%08x\n",t->kcycleCount.val32.upper,
			t->kcycleCount.val32.lower);
	vid_printf("\t\tfileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(t->fileDescs[i] != -1) {
			tVFSNodeNo no = vfs_getNodeNo(t->fileDescs[i]);
			vid_printf("\t\t\t%d : %d",i,t->fileDescs[i]);
			if(vfsn_isValidNodeNo(no)) {
				sVFSNode *n = vfsn_getNode(no);
				if(n && n->parent)
					vid_printf(" (%s->%s)",n->parent->name,n->name);
			}
			vid_printf("\n");
		}
	}
}

void thread_dbg_printState(sThreadRegs *state) {
	vid_printf("\tState:\n",state);
	vid_printf("\t\tesp = 0x%08x\n",state->esp);
	vid_printf("\t\tedi = 0x%08x\n",state->edi);
	vid_printf("\t\tesi = 0x%08x\n",state->esi);
	vid_printf("\t\tebp = 0x%08x\n",state->ebp);
	vid_printf("\t\teflags = 0x%08x\n",state->eflags);
}

#endif
