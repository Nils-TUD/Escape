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
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/pmem.h>
#include <sys/mem/vmm.h>
#include <sys/mem/sllnodes.h>
#include <sys/task/sched.h>
#include <sys/task/lock.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

static sThread *thread_createInitial(sProc *p);
static void thread_initProps(sThread *t);
static void thread_makeUnrunnable(sThread *t);
static tid_t thread_getFreeTid(void);
static bool thread_add(sThread *t);
static void thread_remove(sThread *t);

/* our threads */
static sSLList threads;
static sThread *tidToThread[MAX_THREAD_COUNT];
static tid_t nextTid = 0;
static klock_t threadLock;

sThread *thread_init(sProc *p) {
	sThread *curThread;
	sll_init(&threads,slln_allocNode,slln_freeNode);

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
		t->stackRegions[i] = NULL;
	t->tlsRegion = NULL;
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
	t->resources = 0;
	t->termHeapCount = 0;
	t->termCallbackCount = 0;
	t->termUsageCount = 0;
	t->termLockCount = 0;
	sll_init(&t->reqFrames,slln_allocNode,slln_freeNode);
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
	spinlock_aquire(&threadLock);
	len = sll_length(&threads);
	spinlock_release(&threadLock);
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
	if(t->stackRegions[stackNo] != NULL)
		res = vmm_getRegRange(t->proc->pid,t->stackRegions[stackNo],start,end,false);
	return res;
}

bool thread_getTLSRange(sThread *t,uintptr_t *start,uintptr_t *end) {
	bool res = false;
	if(t->tlsRegion != NULL)
		res = vmm_getRegRange(t->proc->pid,t->tlsRegion,start,end,false);
	return res;
}

sVMRegion *thread_getTLSRegion(const sThread *t) {
	return t->tlsRegion;
}

void thread_setTLSRegion(sThread *t,sVMRegion *vm) {
	t->tlsRegion = vm;
}

bool thread_hasStackRegion(const sThread *t,sVMRegion *vm) {
	size_t i;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		if(t->stackRegions[i] == vm)
			return true;
	}
	return false;
}

void thread_removeRegions(sThread *t,bool remStack) {
	t->tlsRegion = NULL;
	if(remStack) {
		size_t i;
		for(i = 0; i < STACK_REG_COUNT; i++)
			t->stackRegions[i] = NULL;
	}
	/* remove all signal-handler since we've removed the code to handle signals */
	sig_removeHandlerFor(t->tid);
}

int thread_extendStack(uintptr_t address) {
	sThread *t = thread_getRunning();
	size_t i;
	int res = 0;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		/* if it does not yet exist, report an error */
		if(t->stackRegions[i] == NULL)
			return -ENOMEM;

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
	spinlock_aquire(&threadLock);
	threadCount = sll_length(&threads);
	for(n = sll_begin(&threads); n != NULL; n = n->next) {
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
	spinlock_release(&threadLock);
}

void thread_addLock(sThread *cur,klock_t *l) {
	assert(cur->termLockCount < TERM_RESOURCE_CNT);
	cur->termLocks[cur->termLockCount++] = l;
}

void thread_remLock(sThread *cur,A_UNUSED klock_t *l) {
	assert(cur->termLockCount > 0);
	cur->termLockCount--;
}

void thread_addHeapAlloc(sThread *cur,void *ptr) {
	assert(cur->termHeapCount < TERM_RESOURCE_CNT);
	cur->termHeapAllocs[cur->termHeapCount++] = ptr;
}

void thread_remHeapAlloc(sThread *cur,A_UNUSED void *ptr) {
	assert(cur->termHeapCount > 0);
	cur->termHeapCount--;
}

void thread_addFileUsage(sThread *cur,sFile *file) {
	assert(cur->termUsageCount < TERM_RESOURCE_CNT);
	cur->termUsages[cur->termUsageCount++] = file;
}

void thread_remFileUsage(sThread *cur,A_UNUSED sFile *file) {
	assert(cur->termUsageCount > 0);
	cur->termUsageCount--;
}

void thread_addCallback(sThread *cur,fTermCallback callback) {
	assert(cur->termCallbackCount < TERM_RESOURCE_CNT);
	cur->termCallbacks[cur->termCallbackCount++] = callback;
}

void thread_remCallback(sThread *cur,A_UNUSED fTermCallback callback) {
	assert(cur->termCallbackCount > 0);
	cur->termCallbackCount--;
}

bool thread_reserveFrames(sThread *cur,size_t count) {
	while(count > 0) {
		size_t i;
		if(!pmem_reserve(count))
			return false;
		for(i = count; i > 0; i--) {
			frameno_t frm = pmem_allocate(FRM_USER);
			if(!frm)
				break;
			sll_append(&cur->reqFrames,(void*)frm);
			count--;
		}
	}
	return true;
}

frameno_t thread_getFrame(sThread *cur) {
	frameno_t frm = (frameno_t)sll_removeFirst(&cur->reqFrames);
	assert(frm != 0);
	return frm;
}

void thread_discardFrames(sThread *cur) {
	frameno_t frm;
	while((frm = (frameno_t)sll_removeFirst(&cur->reqFrames)) != 0)
		pmem_free(frm,FRM_USER);
}

int thread_create(sThread *src,sThread **dst,sProc *p,uint8_t flags,bool cloneProc) {
	int err = -ENOMEM;
	sThread *t = (sThread*)cache_alloc(sizeof(sThread));
	if(t == NULL)
		return -ENOMEM;

	*(tid_t*)&t->tid = thread_getFreeTid();
	if(t->tid == INVALID_TID) {
		err = -ENOTHREADS;
		goto errThread;
	}
	*(uint8_t*)&t->flags = flags;
	*(sProc**)&t->proc = p;

	thread_initProps(t);
	if(cloneProc) {
		size_t i;
		for(i = 0; i < STACK_REG_COUNT; i++)
			t->stackRegions[i] = vmm_getRegion(p,src->stackRegions[i]->virt);
		if(src->tlsRegion)
			t->tlsRegion = vmm_getRegion(p,src->tlsRegion->virt);
		else
			t->tlsRegion = NULL;
		t->intrptLevel = src->intrptLevel;
		memcpy(t->intrptLevels,src->intrptLevels,sizeof(sIntrptStackFrame*) * MAX_INTRPT_LEVELS);
	}
	else {
		/* add a new tls-region, if its present in the src-thread */
		t->tlsRegion = NULL;
		if(src->tlsRegion != NULL) {
			uintptr_t tlsStart,tlsEnd;
			vmm_getRegRange(src->proc->pid,src->tlsRegion,&tlsStart,&tlsEnd,false);
			err = vmm_add(p->pid,NULL,0,tlsEnd - tlsStart,tlsEnd - tlsStart,REG_TLS,&t->tlsRegion);
			if(err < 0)
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
	return 0;

errAppendIdle:
	sig_removeHandlerFor(t->tid);
	thread_remove(t);
errArch:
	thread_freeArch(t);
errClone:
	if(t->tlsRegion != NULL)
		vmm_remove(p->pid,t->tlsRegion);
errThread:
	cache_free(t);
	return err;
}

void thread_terminate(sThread *t,sThread *cur) {
	/* if its the current one, it can't be chosen again by the scheduler */
	if(t == cur)
		thread_makeUnrunnable(t);
	term_addDead(t);
}

void thread_kill(sThread *t) {
	size_t i;
	/* remove tls */
	if(t->tlsRegion != NULL) {
		vmm_remove(t->proc->pid,t->tlsRegion);
		t->tlsRegion = NULL;
	}

	/* release resources */
	for(i = 0; i < t->termLockCount; i++)
		spinlock_release(t->termLocks[i]);
	for(i = 0; i < t->termHeapCount; i++)
		cache_free(t->termHeapAllocs[i]);
	for(i = 0; i < t->termUsageCount; i++)
		vfs_decUsages(t->termUsages[i]);
	for(i = 0; i < t->termCallbackCount; i++)
		t->termCallbacks[i]();

	/* remove from all modules we may be announced */
	thread_makeUnrunnable(t);
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

static void thread_makeUnrunnable(sThread *t) {
	ev_removeThread(t);
	sched_removeThread(t);
}

void thread_printAll(void) {
	sSLNode *n;
	for(n = sll_begin(&threads); n != NULL; n = n->next) {
		sThread *t = (sThread*)n->data;
		thread_print(t);
	}
}

static const char *thread_getStateName(uint8_t state) {
	static const char *states[] = {
		"UNUSED","RUN","RDY","BLK","ZOM","BLKSWAP","RDYSWAP"
	};
	return states[state];
}

void thread_printShort(const sThread *t) {
	vid_printf("%d [state=%s, prio=%d, cpu=%d, time=%Lums, ev=",
			t->tid,thread_getStateName(t->state),t->priority,t->cpu,thread_getRuntime(t));
	ev_printEvMask(t);
	vid_printf("]");
}

void thread_print(const sThread *t) {
	size_t i;
	sFuncCall *calls;
	vid_printf("Thread %d: (process %d:%s)\n",t->tid,t->proc->pid,t->proc->command);
	vid_printf("\tFlags=%#x\n",t->flags);
	vid_printf("\tState=%s\n",thread_getStateName(t->state));
	vid_printf("\tEvents=");
	ev_printEvMask(t);
	vid_printf("\n");
	vid_printf("\tLastCPU=%d\n",t->cpu);
	vid_printf("\tTlsRegion=%p, ",t->tlsRegion ? t->tlsRegion->virt : 0);
	for(i = 0; i < STACK_REG_COUNT; i++) {
		vid_printf("stackRegion%zu=%p",i,t->stackRegions[i] ? t->stackRegions[i]->virt : 0);
		if(i < STACK_REG_COUNT - 1)
			vid_printf(", ");
	}
	vid_printf("\n");
	vid_printf("\tPriority = %d\n",t->priority);
	vid_printf("\tRuntime = %Lums\n",thread_getRuntime(t));
	vid_printf("\tScheduled = %u\n",t->stats.schedCount);
	vid_printf("\tSyscalls = %u\n",t->stats.syscalls);
	vid_printf("\tCurCycleCount = %Lu\n",t->stats.curCycleCount);
	vid_printf("\tLastCycleCount = %Lu\n",t->stats.lastCycleCount);
	vid_printf("\tcycleStart = %Lu\n",t->stats.cycleStart);
	vid_printf("\tKernel-trace:\n");
	calls = util_getKernelStackTraceOf(t);
	while(calls->addr != 0) {
		vid_printf("\t\t%p -> %p (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
		calls++;
	}
	calls = util_getUserStackTraceOf((sThread*)t);
	if(calls) {
		vid_printf("\tUser-trace:\n");
		while(calls->addr != 0) {
			vid_printf("\t\t%p -> %p (%s)\n",
					(calls + 1)->addr,calls->funcAddr,calls->funcName);
			calls++;
		}
	}
}

static tid_t thread_getFreeTid(void) {
	size_t count = 0;
	tid_t res = INVALID_TID;
	spinlock_aquire(&threadLock);
	while(count < MAX_THREAD_COUNT) {
		if(nextTid >= MAX_THREAD_COUNT)
			nextTid = 0;
		if(tidToThread[nextTid++] == NULL) {
			res = nextTid - 1;
			break;
		}
		count++;
	}
	spinlock_release(&threadLock);
	return res;
}

static bool thread_add(sThread *t) {
	bool res = false;
	spinlock_aquire(&threadLock);
	if(sll_append(&threads,t)) {
		tidToThread[t->tid] = t;
		res = true;
	}
	spinlock_release(&threadLock);
	return res;
}

static void thread_remove(sThread *t) {
	spinlock_aquire(&threadLock);
	sll_removeFirstWith(&threads,t);
	tidToThread[t->tid] = NULL;
	spinlock_release(&threadLock);
}
