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
#include <sys/task/thread.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/task/event.h>
#include <sys/task/timer.h>
#include <sys/task/terminator.h>
#include <sys/vfs/vfs.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/swap.h>
#include <sys/mem/vmm.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/sched.h>
#include <sys/task/lock.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/klock.h>
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>
#include <string.h>
#include <errors.h>

static sThread *thread_createInitial(sProc *p);
static void thread_initProps(sThread *t);
static tid_t thread_getFreeTid(void);
static bool thread_add(sThread *t);
static void thread_remove(sThread *t);

/* our threads */
static sSLList *threads;
static sThread *tidToThread[MAX_THREAD_COUNT];
static tid_t nextTid = 0;
static klock_t threadLock;

sThread *thread_init(sProc *p) {
	sThread *curThread;

	threads = sll_create();
	if(!threads)
		util_panic("Unable to create thread-list");

	/* create thread for init */
	curThread = thread_createInitial(p);
	thread_setRunning(curThread);
	return curThread;
}

static sThread *thread_createInitial(sProc *p) {
	size_t i;
	sThread *t = (sThread*)cache_alloc(sizeof(sThread));
	if(t == NULL)
		util_panic("Unable to allocate mem for initial thread");

	*(uint8_t*)&t->flags = 0;
	*(tid_t*)&t->tid = nextTid++;
	*(sProc**)&t->proc = p;
	thread_initProps(t);
	t->state = ST_RUNNING;
	for(i = 0; i < STACK_REG_COUNT; i++)
		t->stackRegions[i] = -1;
	t->tlsRegion = -1;
	if(thread_initArch(t) < 0)
		util_panic("Unable to init the arch-specific attributes of initial thread");

	/* create list */
	if(!thread_add(t))
		util_panic("Unable to put initial thread into the thread-list");

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid))
		util_panic("Unable to put first thread in vfs");

	return t;
}

static void thread_initProps(sThread *t) {
	t->state = ST_BLOCKED;
	t->newState = ST_READY;
	t->priority = DEFAULT_PRIO;
	t->prioGoodCnt = 0;
	t->lock = 0;
	t->events = 0;
	t->waits = NULL;
	t->ignoreSignals = 0;
	t->signals = NULL;
	t->intrptLevel = 0;
	t->cpu = 0;
	t->stats.timeslice = RUNTIME_UPDATE_INTVAL * 1000;
	t->stats.runtime = 0;
	t->stats.curCycleCount = 0;
	t->stats.lastCycleCount = 0;
	t->stats.cycleStart = cpu_rdtsc();
	t->stats.schedCount = 0;
	t->stats.syscalls = 0;
	sll_init(&t->termHeapAllocs,slln_allocNode,slln_freeNode);
	sll_init(&t->termLocks,slln_allocNode,slln_freeNode);
	sll_init(&t->termUsages,slln_allocNode,slln_freeNode);
}

sIntrptStackFrame *thread_getIntrptStack(const sThread *t) {
	if(t->intrptLevel > 0)
		return t->intrptLevels[t->intrptLevel - 1];
	return NULL;
}

size_t thread_pushIntrptLevel(sThread *t,sIntrptStackFrame *stack) {
	assert(t->intrptLevel < MAX_INTRPT_LEVELS);
	t->intrptLevels[t->intrptLevel++] = stack;
	return t->intrptLevel;
}

void thread_popIntrptLevel(sThread *t) {
	assert(t->intrptLevel > 0);
	t->intrptLevel--;
}

size_t thread_getIntrptLevel(const sThread *t) {
	assert(t->intrptLevel > 0);
	return t->intrptLevel - 1;
}

size_t thread_getCount(void) {
	size_t len;
	klock_aquire(&threadLock);
	len = sll_length(threads);
	klock_release(&threadLock);
	return len;
}

sThread *thread_getById(tid_t tid) {
	if(tid >= ARRAY_SIZE(tidToThread))
		return NULL;
	return tidToThread[tid];
}

void thread_switch(void) {
	thread_doSwitch();
}

void thread_switchNoSigs(void) {
	sThread *t = thread_getRunning();
	/* remember that the current thread wants to ignore signals */
	t->ignoreSignals = 1;
	thread_switch();
	t->ignoreSignals = 0;
}

void thread_block(sThread *t) {
	assert(t != NULL);
	sched_setBlocked(t);
}

void thread_unblock(sThread *t) {
	assert(t != NULL);
	sched_setReady(t);
}

void thread_unblockQuick(sThread *t) {
	sched_setReadyQuick(t);
}

void thread_suspend(sThread *t) {
	assert(t != NULL);
	sched_setSuspended(t,true);
}

void thread_unsuspend(sThread *t) {
	assert(t != NULL);
	sched_setSuspended(t,false);
}

bool thread_getStackRange(sThread *t,uintptr_t *start,uintptr_t *end,size_t stackNo) {
	bool res = false;
	if(t->stackRegions[stackNo] >= 0)
		res = vmm_getRegRange(t->proc->pid,t->stackRegions[stackNo],start,end);
	return res;
}

bool thread_getTLSRange(sThread *t,uintptr_t *start,uintptr_t *end) {
	bool res = false;
	if(t->tlsRegion >= 0)
		res = vmm_getRegRange(t->proc->pid,t->tlsRegion,start,end);
	return res;
}

vmreg_t thread_getTLSRegion(const sThread *t) {
	return t->tlsRegion;
}

void thread_setTLSRegion(sThread *t,vmreg_t rno) {
	t->tlsRegion = rno;
}

bool thread_hasStackRegion(const sThread *t,vmreg_t regNo) {
	size_t i;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		if(t->stackRegions[i] == regNo)
			return true;
	}
	return false;
}

void thread_removeRegions(sThread *t,bool remStack) {
	klock_aquire(&t->lock);
	t->tlsRegion = -1;
	if(remStack) {
		size_t i;
		for(i = 0; i < STACK_REG_COUNT; i++)
			t->stackRegions[i] = -1;
	}
	/* remove all signal-handler since we've removed the code to handle signals */
	sig_removeHandlerFor(t->tid);
	klock_release(&t->lock);
}

int thread_extendStack(uintptr_t address) {
	sThread *t = thread_getRunning();
	size_t i;
	int res = 0;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		/* if it does not yet exist, report an error */
		if(t->stackRegions[i] < 0)
			return ERR_NOT_ENOUGH_MEM;

		res = vmm_growStackTo(t->proc->pid,t->stackRegions[i],address);
		if(res >= 0)
			return res;
	}
	return res;
}

uint64_t thread_getCycles(const sThread *t) {
	return t->stats.lastCycleCount;
}

void thread_updateRuntimes(void) {
	sSLNode *n;
	size_t threadCount;
	uint64_t cyclesPerSec = cpu_getSpeed();
	klock_aquire(&threadLock);
	threadCount = sll_length(threads);
	for(n = sll_begin(threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		/* update cycle-stats */
		if(t->state == ST_RUNNING) {
			/* we want to measure the last second only */
			uint64_t cycles = cpu_rdtsc() - t->stats.cycleStart;
			t->stats.lastCycleCount = t->stats.curCycleCount + MIN(cyclesPerSec,cycles);
		}
		else
			t->stats.lastCycleCount = t->stats.curCycleCount;
		t->stats.curCycleCount = 0;

		/* raise/lower the priority if necessary */
		sched_adjustPrio(t,threadCount);
	}
	klock_release(&threadLock);
}

void thread_addLock(sThread *cur,klock_t *l) {
	sll_append(&cur->termLocks,l);
}

void thread_remLock(sThread *cur,klock_t *l) {
	sll_removeFirstWith(&cur->termLocks,l);
}

void thread_addHeapAlloc(sThread *cur,void *ptr) {
	sll_append(&cur->termHeapAllocs,ptr);
}

void thread_remHeapAlloc(sThread *cur,void *ptr) {
	sll_removeFirstWith(&cur->termHeapAllocs,ptr);
}

void thread_addFileUsage(sThread *cur,file_t file) {
	sll_append(&cur->termUsages,(void*)file);
}

void thread_remFileUsage(sThread *cur,file_t file) {
	sll_removeFirstWith(&cur->termUsages,(void*)file);
}

int thread_create(sThread *src,sThread **dst,sProc *p,uint8_t flags,bool cloneProc) {
	int err = ERR_NOT_ENOUGH_MEM;
	sThread *t = (sThread*)cache_alloc(sizeof(sThread));
	if(t == NULL)
		return ERR_NOT_ENOUGH_MEM;

	klock_aquire(&src->lock);
	*(tid_t*)&t->tid = thread_getFreeTid();
	if(t->tid == INVALID_TID) {
		err = ERR_NO_FREE_THREADS;
		goto errThread;
	}
	*(uint8_t*)&t->flags = flags;
	*(sProc**)&t->proc = p;

	thread_initProps(t);
	if(cloneProc) {
		size_t i;
		for(i = 0; i < STACK_REG_COUNT; i++)
			t->stackRegions[i] = src->stackRegions[i];
		t->tlsRegion = src->tlsRegion;
		t->intrptLevel = src->intrptLevel;
		memcpy(t->intrptLevels,src->intrptLevels,sizeof(sIntrptStackFrame*) * MAX_INTRPT_LEVELS);
	}
	else {
		/* add a new tls-region, if its present in the src-thread */
		t->tlsRegion = -1;
		if(src->tlsRegion >= 0) {
			uintptr_t tlsStart,tlsEnd;
			vmm_getRegRange(src->proc->pid,src->tlsRegion,&tlsStart,&tlsEnd);
			t->tlsRegion = vmm_add(p->pid,NULL,0,tlsEnd - tlsStart,tlsEnd - tlsStart,REG_TLS);
			if(t->tlsRegion < 0)
				goto errThread;
		}
	}

	/* clone architecture-specific stuff */
	if((err = thread_createArch(src,t,cloneProc)) < 0)
		goto errClone;

	/* insert into thread-list */
	if(!thread_add(t))
		goto errArch;

	/* append to idle-list if its an idle-thread */
	if(flags & T_IDLE)
		sched_addIdleThread(t);

	/* clone signal-handler (here because the thread needs to be in the map first) */
	if(cloneProc)
		sig_cloneHandler(src->tid,t->tid);

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->tid))
		goto errAppendIdle;

	*dst = t;
	klock_release(&src->lock);
	return 0;

errAppendIdle:
	sig_removeHandlerFor(t->tid);
	thread_remove(t);
errArch:
	thread_freeArch(t);
errClone:
	if(t->tlsRegion >= 0)
		vmm_remove(p->pid,t->tlsRegion);
errThread:
	cache_free(t);
	klock_release(&src->lock);
	return err;
}

void thread_terminate(sThread *t) {
	/* just remove us from scheduler; he will take care that we don't get chosen again. */
	/* if we are currently running on a different cpu, it will be interrupted and asked to
	 * terminate the thread */
	sched_removeThread(t);
	term_addDead(t);
}

void thread_kill(sThread *t) {
	sSLNode *n;
	/* remove tls */
	if(t->tlsRegion >= 0) {
		vmm_remove(t->proc->pid,t->tlsRegion);
		t->tlsRegion = -1;
	}

	/* release resources */
	for(n = sll_begin(&t->termLocks); n != NULL; n = n->next)
		klock_release((klock_t*)n->data);
	sll_clear(&t->termLocks,false);
	for(n = sll_begin(&t->termHeapAllocs); n != NULL; n = n->next)
		cache_free(n->data);
	sll_clear(&t->termHeapAllocs,false);
	for(n = sll_begin(&t->termUsages); n != NULL; n = n->next)
		vfs_decUsages((file_t)n->data);
	sll_clear(&t->termUsages,false);

	/* remove from all modules we may be announced */
	ev_removeThread(t);
	sched_removeThread(t);
	timer_removeThread(t->tid);
	sig_removeHandlerFor(t->tid);
	thread_freeArch(t);
	vfs_removeThread(t->tid);

	/* notify the process about it */
	ev_wakeup(EVI_THREAD_DIED,(evobj_t)t->proc);

	/* finally, destroy thread */
	thread_remove(t);
	cache_free(t);
}

void thread_printAll(void) {
	sSLNode *n;
	vid_printf("Threads:\n");
	for(n = sll_begin(threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		thread_print(t);
	}
}

void thread_print(const sThread *t) {
	size_t i;
	sFuncCall *calls;
	static const char *states[] = {
		"UNUSED","RUNNING","READY","BLOCKED","ZOMBIE","BLOCKEDSWAP","READYSWAP"
	};
	vid_printf("\tThread %d: (process %d:%s)\n",t->tid,t->proc->pid,t->proc->command);
	vid_printf("\t\tFlags=%#x\n",t->flags);
	vid_printf("\t\tState=%s\n",states[t->state]);
	vid_printf("\t\tEvents=");
	ev_printEvMask(t);
	vid_printf("\n");
	vid_printf("\t\tLastCPU=%d\n",t->cpu);
	vid_printf("\t\tTlsRegion=%d, ",t->tlsRegion);
	for(i = 0; i < STACK_REG_COUNT; i++) {
		vid_printf("stackRegion%zu=%d",i,t->stackRegions[i]);
		if(i < STACK_REG_COUNT - 1)
			vid_printf(", ");
	}
	vid_printf("\n");
	vid_printf("\t\tRuntime = %Lums\n",thread_getRuntime(t));
	vid_printf("\t\tCurCycleCount = %Lu\n",t->stats.curCycleCount);
	vid_printf("\t\tLastCycleCount = %Lu\n",t->stats.lastCycleCount);
	vid_printf("\t\tcycleStart = %Lu\n",t->stats.cycleStart);
	vid_printf("\t\tKernel-trace:\n");
	calls = util_getKernelStackTraceOf(t);
	while(calls->addr != 0) {
		vid_printf("\t\t\t%p -> %p (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
		calls++;
	}
	calls = util_getUserStackTraceOf((sThread*)t);
	if(calls) {
		vid_printf("\t\tUser-trace:\n");
		while(calls->addr != 0) {
			vid_printf("\t\t\t%p -> %p (%s)\n",
					(calls + 1)->addr,calls->funcAddr,calls->funcName);
			calls++;
		}
	}
}

static tid_t thread_getFreeTid(void) {
	size_t count = 0;
	tid_t res = INVALID_TID;
	klock_aquire(&threadLock);
	while(count < MAX_THREAD_COUNT) {
		if(nextTid >= MAX_THREAD_COUNT)
			nextTid = 0;
		if(tidToThread[nextTid++] == NULL) {
			res = nextTid - 1;
			break;
		}
		count++;
	}
	klock_release(&threadLock);
	return res;
}

static bool thread_add(sThread *t) {
	bool res = false;
	klock_aquire(&threadLock);
	if(sll_append(threads,t)) {
		tidToThread[t->tid] = t;
		res = true;
	}
	klock_release(&threadLock);
	return res;
}

static void thread_remove(sThread *t) {
	klock_aquire(&threadLock);
	sll_removeFirstWith(threads,t);
	tidToThread[t->tid] = NULL;
	klock_release(&threadLock);
}
