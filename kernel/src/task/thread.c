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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/machine/cpu.h>
#include <sys/machine/gdt.h>
#include <sys/machine/timer.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/info.h>
#include <sys/vfs/node.h>
#include <sys/mem/kheap.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/task/sched.h>
#include <sys/task/lock.h>
#include <sys/util.h>
#include <sys/debug.h>
#include <sys/kevent.h>
#include <sys/video.h>
#include <esc/hashmap.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

#define THREAD_MAP_SIZE		1024

/**
 * For creating the init-thread and idle-thread
 *
 * @param p the process
 * @param state the desired state
 * @return the created thread
 */
static sThread *thread_createInitial(sProc *p,eThreadState state);

/* our map for the threads */
static sHashMap *threads;
static sSLList *threadMap[THREAD_MAP_SIZE] = {NULL};
static sThread *cur = NULL;
static tTid nextTid = 0;
static volatile tTid runnableThread = INVALID_TID;

/* list of dead threads that should be destroyed */
static sSLList* deadThreads = NULL;

static u32 thread_getkey(const void *data) {
	return ((sThread*)data)->tid;
}

sThread *thread_init(sProc *p) {
	threads = hm_create(threadMap,THREAD_MAP_SIZE,thread_getkey);
	assert(threads);

	/* create idle-thread */
	thread_createInitial(p,ST_BLOCKED);
	/* create thread for init */
	cur = thread_createInitial(p,ST_RUNNING);
	return cur;
}

static sThread *thread_createInitial(sProc *p,eThreadState state) {
	tFD i;
	sThread *t = (sThread*)kheap_alloc(sizeof(sThread));
	if(t == NULL)
		util_panic("Unable to allocate mem for initial thread");

	t->state = state;
	t->events = EV_NOEVENT;
	t->waitsInKernel = 0;
	t->tid = nextTid++;
	t->proc = p;
	/* we'll give the thread a stack later */
	t->stats.ucycleCount.val64 = 0;
	t->stats.ucycleStart = 0;
	t->stats.kcycleCount.val64 = 0;
	t->stats.kcycleStart = 0;
	t->stats.schedCount = 0;
	t->stats.input = 0;
	t->stats.output = 0;
	t->fpuState = NULL;
	t->signal = 0;
	t->tlsRegion = -1;
	/* init fds */
	for(i = 0; i < MAX_FD_COUNT; i++)
		t->fileDescs[i] = -1;

	/* create list */
	if(!hm_add(threads,t))
		util_panic("Unable to put initial thread into the thread-map");

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid))
		util_panic("Unable to put first thread in vfs");

	return t;
}

u32 thread_getCount(void) {
	return hm_getCount(threads);
}

sThread *thread_getRunning(void) {
	return cur;
}

sThread *thread_getById(tTid tid) {
	return hm_get(threads,tid);
}

void thread_switch(void) {
	thread_switchTo(sched_perform()->tid);
}

void thread_switchTo(tTid tid) {
	/* if we're idling we have to leave this interrupt first and stop idling then */
	if(cur->tid == IDLE_TID) {
		/* ignore it if we should continue idling */
		if(tid == IDLE_TID)
			return;
		/* if we've already stored a thread to run, it is no longer in the ready-queue of the
		 * scheduler. so we have to put it back into it. */
		if(runnableThread != INVALID_TID) {
			sThread *last = thread_getById(runnableThread);
			/* sched_setReady() will do nothing if state is ST_READY */
			last->state = ST_RUNNING;
			sched_setReady(last);
		}
		/* store to which thread we should switch */
		runnableThread = tid;
		return;
	}

	/* finish kernel-time here since we're switching the process */
	if(tid != cur->tid) {
		u64 kcstart = cur->stats.kcycleStart;
		if(kcstart > 0) {
			u64 cycles = cpu_rdtsc();
			cur->stats.kcycleCount.val64 += cycles - kcstart;
		}

		if(!thread_save(&cur->save)) {
			u32 tlsStart,tlsEnd;
			sThread *old;
			sThread *t = thread_getById(tid);
			vassert(t != NULL,"Thread with tid %d not found",tid);

			/* mark old process ready, if it should not be blocked, killed or something */
			if(cur->state == ST_RUNNING)
				sched_setReady(cur);

			old = cur;
			cur = t;

			/* if it is the idle-thread, stay here and wait for an interrupt */
			if(cur->tid == IDLE_TID) {
				/* user-mode starts here */
				cur->stats.ucycleStart = cpu_rdtsc();
				/* idle until there is another thread to run */
				__asm__("sti");
				while(runnableThread == INVALID_TID)
					__asm__("hlt");

				/* now a thread wants to run */
				__asm__("cli");
				cur = thread_getById(runnableThread);
				runnableThread = INVALID_TID;
			}

			/* set used */
			cur->stats.schedCount++;
			vmm_setTimestamp(cur,timer_getTimestamp());
			sched_setRunning(cur);

			if(old->proc != cur->proc) {
				/* remove the io-map. it will be set as soon as the process accesses an io-port
				 * (we'll get an exception) */
				tss_removeIOMap();
				tss_setStackPtr(cur->proc->flags & P_VM86);
				paging_setCur(cur->proc->pagedir);
			}

			/* set TLS-segment in GDT */
			if(cur->tlsRegion >= 0) {
				vmm_getRegRange(cur->proc,cur->tlsRegion,&tlsStart,&tlsEnd);
				gdt_setTLS(tlsStart,tlsEnd - tlsStart);
			}
			else
				gdt_setTLS(0,0xFFFFFFFF);
			/* lock the FPU so that we can save the FPU-state for the previous process as soon
			 * as this one wants to use the FPU */
			fpu_lockFPU();
			thread_resume(cur->proc->pagedir,&cur->save,
					sll_length(cur->proc->threads) > 1 ? cur->kstackFrame : 0);
		}

		/* now start kernel-time again */
		cur->stats.kcycleStart = cpu_rdtsc();
	}

	/* destroy threads, if there are any */
	if(deadThreads != NULL) {
		sSLNode *n;
		sThread *t;
		for(n = sll_begin(deadThreads); n != NULL; n = n->next) {
			/* we want to destroy the stacks here because if the whole process is destroyed
			 * the proc-module doesn't kill the running thread anyway so there will never
			 * be the case that we should destroy a single thread later */
			t = (sThread*)n->data;
			thread_kill(t);
		}
		sll_destroy(deadThreads,false);
		deadThreads = NULL;
	}
}

void thread_switchNoSigs(void) {
	/* remember that the current thread waits in the kernel */
	/* atm this is just used by the signal-module to check whether we can send a signal to a
	 * thread or not */
	cur->waitsInKernel = 1;
	thread_switch();
	cur->waitsInKernel = 0;
	kev_notify(KEV_KWAIT_DONE,cur->tid);
}

void thread_wait(tTid tid,void *mask,u16 events) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL,"Thread with id %d not found",tid);
	t->events = ((u32)mask << 16) | events;
	sched_setBlocked(t);
}

void thread_wakeupAll(void *mask,u16 event) {
	sched_unblockAll((u32)mask & 0xFFFF,event);
}

void thread_wakeup(tTid tid,u16 event) {
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

bool thread_setReady(tTid tid) {
	sThread *t = thread_getById(tid);
	vassert(t != NULL && t != cur,"tid=%d, pid=%d, cmd=%s",
			t ? t->tid : 0,t ? t->proc->pid : 0,t ? t->proc->command : "?");
	if(!t->waitsInKernel)
		sched_setReady(t);
	return t->state == ST_READY;
}

bool thread_setBlocked(tTid tid) {
	sThread *t = thread_getById(tid);
	assert(t != NULL);
	sched_setBlocked(t);
	return t->state == ST_BLOCKED;
}

void thread_setSuspended(tTid tid,bool blocked) {
	sThread *t = thread_getById(tid);
	assert(t != NULL);
	sched_setSuspended(t,blocked);
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
	return vmm_growStackTo(cur,address);
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
	t->stats.kcycleCount.val64 = 0;
	t->stats.kcycleStart = 0;
	t->stats.ucycleCount.val64 = 0;
	t->stats.ucycleStart = 0;
	t->stats.input = 0;
	t->stats.output = 0;
	t->stats.schedCount = 0;
	t->proc = p;
	t->signal = 0;
	if(cloneProc) {
		/* proc_clone() sets t->kstackFrame in this case */
		t->stackRegion = src->stackRegion;
		t->tlsRegion = src->tlsRegion;

		/* clone signal-handler */
		if(sig_cloneHandler(src->tid,t->tid) < 0)
			goto errThread;
	}
	else {
		/* no swapping here because we don't want to make a thread-switch */
		/* TODO */
		u32 neededFrms = 0;/*paging_countFramesForMap(t->ustackBegin - t->ustackPages * PAGE_SIZE,
				INITIAL_STACK_PAGES);*/
		if(mm_getFreeFrames(MM_DEF) < 1 + neededFrms)
			goto errThread;

		/* add a new stack-region */
		t->stackRegion = vmm_add(p,NULL,0,INITIAL_STACK_PAGES * PAGE_SIZE,
				INITIAL_STACK_PAGES * PAGE_SIZE,REG_STACK);
		if(t->stackRegion < 0)
			goto errThread;
		/* add kernel-stack */
		*stackFrame = t->kstackFrame = mm_allocate();
		p->ownFrames++;
		/* add a new tls-region, if its present in the src-thread */
		t->tlsRegion = -1;
		if(src->tlsRegion >= 0) {
			u32 tlsStart,tlsEnd;
			vmm_getRegRange(src->proc,src->tlsRegion,&tlsStart,&tlsEnd);
			t->tlsRegion = vmm_add(p,NULL,0,tlsEnd - tlsStart,tlsEnd - tlsStart,REG_TLS);
			if(t->tlsRegion < 0)
				goto errStack;
		}
	}

	/* insert into thread-map */
	if(!hm_add(threads,t))
		goto errClone;

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid))
		goto errAppend;

	/* inherit file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		t->fileDescs[i] = src->fileDescs[i];
		if(t->fileDescs[i] != -1)
			t->fileDescs[i] = vfs_inheritFileNo(t->tid,t->fileDescs[i]);
	}

	*dst = t;
	return 0;

errAppend:
	hm_remove(threads,t);
errClone:
	if(t->tlsRegion >= 0)
		vmm_remove(p,t->tlsRegion);
	else if(cloneProc)
		sig_removeHandlerFor(t->tid);
errStack:
	if(!cloneProc) {
		mm_free(t->kstackFrame);
		p->ownFrames--;
		vmm_remove(p,t->stackRegion);
	}
errThread:
	kheap_free(t);
	return ERR_NOT_ENOUGH_MEM;
}

void thread_kill(sThread *t) {
	tFD i;
	/* we can't destroy the current thread */
	if(t == cur) {
		/* put it in the dead-thread-queue to destroy it later */
		if(deadThreads == NULL) {
			deadThreads = sll_create();
			if(deadThreads == NULL)
				util_panic("Not enough mem for dead thread-list");
		}
		if(!sll_append(deadThreads,t))
			util_panic("Not enough mem to append dead thread");
		/* remove from scheduler and ensure that he don't picks us again */
		sched_removeThread(t);
		/* remove from timer, too, so that we don't get waked up again */
		timer_removeThread(t->tid);
		t->state = ST_ZOMBIE;
		return;
	}

	/* remove tls */
	if(t->tlsRegion >= 0) {
		vmm_remove(t->proc,t->tlsRegion);
		t->tlsRegion = -1;
	}
	if(t->stackRegion >= 0) {
		/* remove stack-region */
		vmm_remove(t->proc,t->stackRegion);
		t->stackRegion = -1;
	}
	/* free kernel-stack */
	mm_free(t->kstackFrame);
	t->proc->ownFrames--;

	/* release file-descriptors */
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(t->fileDescs[i] != -1) {
			vfs_closeFile(t->tid,t->fileDescs[i]);
			t->fileDescs[i] = -1;
		}
	}

	/* remove from process */
	sll_removeFirst(t->proc->threads,t);
	/* if there is just one thread left we have to map his kernel-stack again because we won't
	 * do it for single-thread-processes on a switch for performance-reasons */
	if(sll_length(t->proc->threads) == 1) {
		u32 stackFrame = ((sThread*)sll_get(t->proc->threads,0))->kstackFrame;
		paging_mapTo(t->proc->pagedir,KERNEL_STACK,&stackFrame,1,
				PG_PRESENT | PG_WRITABLE | PG_SUPERVISOR);
	}

	/* remove from all modules we may be announced */
	sched_removeThread(t);
	timer_removeThread(t->tid);
	fpu_freeState(&t->fpuState);
	sig_removeHandlerFor(t->tid);
	vfs_removeThread(t->tid);
	lock_releaseAll(t->tid);

	/* notify others that wait for dying threads */
	sig_addSignal(SIG_THREAD_DIED,t->tid);
	thread_wakeupAll(t->proc,EV_THREAD_DIED);

	/* finally, destroy thread */
	hm_remove(threads,t);
	kheap_free(t);
}

/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void thread_dbg_printAll(void) {
	sThread *t;
	vid_printf("Threads:\n");
	for(hm_begin(threads); (t = hm_next(threads)); )
		thread_dbg_print(t);
}

void thread_dbg_print(sThread *t) {
	sFuncCall *calls;
	static const char *states[] = {
		"UNUSED","RUNNING","READY","BLOCKED","ZOMBIE","BLOCKEDSWAP","READYSWAP"
	};
	tFD i;
	vid_printf("\tthread %d: (process %d:%s)\n",t->tid,t->proc->pid,t->proc->command);
	vid_printf("\t\tstate=%s\n",states[t->state]);
	vid_printf("\t\tevents=%x\n",t->events);
	vid_printf("\t\tkstackFrame=0x%x\n",t->kstackFrame);
	vid_printf("\t\tucycleCount = 0x%08x%08x\n",t->stats.ucycleCount.val32.upper,
			t->stats.ucycleCount.val32.lower);
	vid_printf("\t\tkcycleCount = 0x%08x%08x\n",t->stats.kcycleCount.val32.upper,
			t->stats.kcycleCount.val32.lower);
	vid_printf("\t\tfileDescs:\n");
	for(i = 0; i < MAX_FD_COUNT; i++) {
		if(t->fileDescs[i] != -1) {
			tInodeNo ino;
			tDevNo dev;
			if(vfs_getFileId(t->fileDescs[i],&ino,&dev) == 0) {
				vid_printf("\t\t\t%d : %d",i,t->fileDescs[i]);
				if(dev == VFS_DEV_NO && vfsn_isValidNodeNo(ino))
					vid_printf(" (%s)",vfsn_getPath(ino));
				vid_printf("\n");
			}
		}
	}
	vid_printf("\t\tkernel-trace:\n");
	calls = util_getKernelStackTraceOf(t);
	while(calls->addr != 0) {
		vid_printf("\t\t\t%#08x -> %#08x (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
		calls++;
	}
	calls = util_getUserStackTraceOf(t);
	if(calls) {
		vid_printf("\t\tuser-trace:\n");
		while(calls->addr != 0) {
			vid_printf("\t\t\t%#08x -> %#08x (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
			calls++;
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
