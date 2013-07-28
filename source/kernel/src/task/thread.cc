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
#include <sys/mem/physmem.h>
#include <sys/mem/virtmem.h>
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

/* our threads */
sSLList ThreadBase::threads;
Thread *ThreadBase::tidToThread[MAX_THREAD_COUNT];
tid_t ThreadBase::nextTid = 0;
klock_t ThreadBase::threadLock;

Thread *ThreadBase::init(Proc *p) {
	Thread *curThread;
	sll_init(&threads,slln_allocNode,slln_freeNode);

	/* create thread for init */
	curThread = createInitial(p);
	setRunning(curThread);
	return curThread;
}

Thread *ThreadBase::createInitial(Proc *p) {
	size_t i;
	Thread *t = (Thread*)Cache::alloc(sizeof(Thread));
	if(t == NULL)
		Util::panic("Unable to allocate mem for initial thread");

	t->tid = nextTid++;
	t->proc = p;
	t->flags = 0;
	t->initProps();
	t->state = Thread::RUNNING;
	for(i = 0; i < STACK_REG_COUNT; i++)
		t->stackRegions[i] = NULL;
	t->tlsRegion = NULL;
	if(initArch(t) < 0)
		Util::panic("Unable to init the arch-specific attributes of initial thread");

	/* create list */
	if(!t->add())
		Util::panic("Unable to put initial thread into the thread-list");

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->getTid()))
		Util::panic("Unable to put first thread in vfs");

	return t;
}

void ThreadBase::initProps() {
	state = Thread::BLOCKED;
	newState = Thread::READY;
	priority = DEFAULT_PRIO;
	prioGoodCnt = 0;
	events = 0;
	waits = NULL;
	ignoreSignals = 0;
	signals = NULL;
	intrptLevel = 0;
	cpu = 0;
	stats.timeslice = RUNTIME_UPDATE_INTVAL * 1000;
	stats.runtime = 0;
	stats.curCycleCount = 0;
	stats.lastCycleCount = 0;
	stats.cycleStart = CPU::rdtsc();
	stats.schedCount = 0;
	stats.syscalls = 0;
	resources = 0;
	termHeapCount = 0;
	termCallbackCount = 0;
	termUsageCount = 0;
	termLockCount = 0;
	sll_init(&reqFrames,slln_allocNode,slln_freeNode);
}

size_t ThreadBase::getCount() {
	size_t len;
	spinlock_aquire(&threadLock);
	len = sll_length(&threads);
	spinlock_release(&threadLock);
	return len;
}

int ThreadBase::extendStack(uintptr_t address) {
	Thread *t = Thread::getRunning();
	size_t i;
	int res = 0;
	for(i = 0; i < STACK_REG_COUNT; i++) {
		/* if it does not yet exist, report an error */
		if(t->stackRegions[i] == NULL)
			return -ENOMEM;

		res = t->getProc()->getVM()->growStackTo(t->stackRegions[i],address);
		if(res >= 0)
			return res;
	}
	return res;
}

bool ThreadBase::getStackRange(uintptr_t *start,uintptr_t *end,size_t stackNo) {
	bool res = false;
	if(stackRegions[stackNo] != NULL)
		res = proc->getVM()->getRegRange(stackRegions[stackNo],start,end,false);
	return res;
}

bool ThreadBase::getTLSRange(uintptr_t *start,uintptr_t *end) {
	bool res = false;
	if(tlsRegion != NULL)
		res = proc->getVM()->getRegRange(tlsRegion,start,end,false);
	return res;
}

void ThreadBase::updateRuntimes(void) {
	sSLNode *n;
	size_t threadCount;
	uint64_t cyclesPerSec = Thread::ticksPerSec();
	spinlock_aquire(&threadLock);
	threadCount = sll_length(&threads);
	for(n = sll_begin(&threads); n != NULL; n = n->next) {
		Thread *t = (Thread*)n->data;
		/* update cycle-stats */
		if(t->state == Thread::RUNNING) {
			/* we want to measure the last second only */
			uint64_t cycles = Thread::getTSC() - t->stats.cycleStart;
			t->stats.lastCycleCount = t->stats.curCycleCount + MIN(cyclesPerSec,cycles);
		}
		else
			t->stats.lastCycleCount = t->stats.curCycleCount;
		t->stats.curCycleCount = 0;

		/* raise/lower the priority if necessary */
		Sched::adjustPrio(t,threadCount);
	}
	spinlock_release(&threadLock);
}

bool ThreadBase::reserveFrames(size_t count) {
	while(count > 0) {
		size_t i;
		if(!PhysMem::reserve(count))
			return false;
		for(i = count; i > 0; i--) {
			frameno_t frm = PhysMem::allocate(PhysMem::USR);
			if(!frm)
				break;
			sll_append(&reqFrames,(void*)frm);
			count--;
		}
	}
	return true;
}

int ThreadBase::create(Thread *src,Thread **dst,Proc *p,uint8_t flags,bool cloneProc) {
	int err = -ENOMEM;
	Thread *t = (Thread*)Cache::alloc(sizeof(Thread));
	if(t == NULL)
		return -ENOMEM;

	t->tid = getFreeTid();
	if(t->getTid() == INVALID_TID) {
		err = -ENOTHREADS;
		goto errThread;
	}
	t->proc = p;
	t->flags = flags;

	t->initProps();
	if(cloneProc) {
		size_t i;
		for(i = 0; i < STACK_REG_COUNT; i++) {
			if(src->stackRegions[i])
				t->stackRegions[i] = p->getVM()->getRegion(src->stackRegions[i]->virt);
			else
				t->stackRegions[i] = NULL;
		}
		if(src->tlsRegion)
			t->tlsRegion = p->getVM()->getRegion(src->tlsRegion->virt);
		else
			t->tlsRegion = NULL;
		t->intrptLevel = src->intrptLevel;
		memcpy(t->intrptLevels,src->intrptLevels,sizeof(IntrptStackFrame*) * MAX_INTRPT_LEVELS);
	}
	else {
		/* add a new tls-region, if its present in the src-thread */
		t->tlsRegion = NULL;
		if(src->tlsRegion != NULL) {
			uintptr_t tlsStart,tlsEnd;
			src->getProc()->getVM()->getRegRange(src->tlsRegion,&tlsStart,&tlsEnd,false);
			err = p->getVM()->map(0,tlsEnd - tlsStart,0,PROT_READ | PROT_WRITE,0,NULL,0,&t->tlsRegion);
			if(err < 0)
				goto errThread;
		}
	}

	/* clone architecture-specific stuff */
	if((err = createArch(src,t,cloneProc)) < 0)
		goto errClone;

	/* insert into thread-list */
	if(!t->add())
		goto errArch;

	/* append to idle-list if its an idle-thread */
	if(flags & T_IDLE)
		Sched::addIdleThread(t);

	/* clone signal-handler (here because the thread needs to be in the map first) */
	if(cloneProc)
		Signals::cloneHandler(src->getTid(),t->getTid());

	/* insert in VFS; thread needs to be inserted for it */
	if(!vfs_createThread(t->getTid()))
		goto errAppendIdle;

	*dst = t;
	return 0;

errAppendIdle:
	Signals::removeHandlerFor(t->getTid());
	t->remove();
errArch:
	freeArch(t);
errClone:
	if(t->tlsRegion != NULL)
		p->getVM()->remove(t->tlsRegion);
errThread:
	Cache::free(t);
	return err;
}

void ThreadBase::kill() {
	size_t i;
	assert(state == Thread::ZOMBIE);
	/* remove tls */
	if(tlsRegion != NULL) {
		proc->getVM()->remove(tlsRegion);
		tlsRegion = NULL;
	}

	/* release resources */
	for(i = 0; i < termLockCount; i++)
		spinlock_release(termLocks[i]);
	for(i = 0; i < termHeapCount; i++)
		Cache::free(termHeapAllocs[i]);
	for(i = 0; i < termUsageCount; i++)
		vfs_decUsages(termUsages[i]);
	for(i = 0; i < termCallbackCount; i++)
		termCallbacks[i]();

	/* remove from all modules we may be announced */
	makeUnrunnable();
	Timer::removeThread(tid);
	Signals::removeHandlerFor(tid);
	freeArch(static_cast<Thread*>(this));
	vfs_removeThread(tid);

	/* notify the process about it */
	Event::wakeup(EVI_THREAD_DIED,(evobj_t)proc);

	/* finally, destroy thread */
	remove();
	Cache::free(this);
}

void ThreadBase::makeUnrunnable() {
	Event::removeThread(static_cast<Thread*>(this));
	Sched::removeThread(static_cast<Thread*>(this));
}

void ThreadBase::printAll() {
	sSLNode *n;
	for(n = sll_begin(&threads); n != NULL; n = n->next) {
		Thread *t = (Thread*)n->data;
		t->print();
	}
}

static const char *getStateName(uint8_t state) {
	static const char *states[] = {
		"UNUSED","RUN","RDY","BLK","ZOM","BLKSWAP","RDYSWAP"
	};
	return states[state];
}

void ThreadBase::printShort() const {
	Video::printf("%d [state=%s, prio=%d, cpu=%d, time=%Lums, ev=",
			tid,getStateName(state),priority,cpu,getRuntime());
	Event::printEvMask(static_cast<const Thread*>(this));
	Video::printf("]");
}

void ThreadBase::print() const {
	size_t i;
	Util::FuncCall *calls;
	Video::printf("Thread %d: (process %d:%s)\n",tid,proc->getPid(),proc->getCommand());
	prf_pushIndent();
	Video::printf("Flags=%#x\n",flags);
	Video::printf("State=%s\n",getStateName(state));
	Video::printf("Events=");
	Event::printEvMask(static_cast<const Thread*>(this));
	Video::printf("\n");
	Video::printf("LastCPU=%d\n",cpu);
	Video::printf("TlsRegion=%p, ",tlsRegion ? tlsRegion->virt : 0);
	for(i = 0; i < STACK_REG_COUNT; i++) {
		Video::printf("stackRegion%zu=%p",i,stackRegions[i] ? stackRegions[i]->virt : 0);
		if(i < STACK_REG_COUNT - 1)
			Video::printf(", ");
	}
	Video::printf("\n");
	Video::printf("Priority = %d\n",priority);
	Video::printf("Runtime = %Lums\n",getRuntime());
	Video::printf("Scheduled = %u\n",stats.schedCount);
	Video::printf("Syscalls = %u\n",stats.syscalls);
	Video::printf("CurCycleCount = %Lu\n",stats.curCycleCount);
	Video::printf("LastCycleCount = %Lu\n",stats.lastCycleCount);
	Video::printf("cycleStart = %Lu\n",stats.cycleStart);
	Video::printf("Kernel-trace:\n");
	prf_pushIndent();
	calls = Util::getKernelStackTraceOf(static_cast<const Thread*>(this));
	while(calls->addr != 0) {
		Video::printf("%p -> %p (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
		calls++;
	}
	prf_popIndent();
	calls = Util::getUserStackTraceOf(const_cast<Thread*>(static_cast<const Thread*>(this)));
	if(calls) {
		Video::printf("User-trace:\n");
		prf_pushIndent();
		while(calls->addr != 0) {
			Video::printf("%p -> %p (%s)\n",
					(calls + 1)->addr,calls->funcAddr,calls->funcName);
			calls++;
		}
		prf_popIndent();
	}
	Util::printUserStateOf(static_cast<const Thread*>(this));
	prf_popIndent();
}

tid_t ThreadBase::getFreeTid(void) {
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

bool ThreadBase::add() {
	bool res = false;
	spinlock_aquire(&threadLock);
	if(sll_append(&threads,this)) {
		tidToThread[tid] = static_cast<Thread*>(this);
		res = true;
	}
	spinlock_release(&threadLock);
	return res;
}

void ThreadBase::remove() {
	spinlock_aquire(&threadLock);
	sll_removeFirstWith(&threads,this);
	tidToThread[tid] = NULL;
	spinlock_release(&threadLock);
}
