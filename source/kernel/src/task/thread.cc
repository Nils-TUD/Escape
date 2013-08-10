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
#include <sys/vfs/openfile.h>
#include <sys/mem/cache.h>
#include <sys/mem/paging.h>
#include <sys/mem/physmem.h>
#include <sys/mem/virtmem.h>
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
ISList<Thread*> ThreadBase::threads;
Thread *ThreadBase::tidToThread[MAX_THREAD_COUNT];
tid_t ThreadBase::nextTid = 0;
klock_t ThreadBase::lock;

Thread *ThreadBase::init(Proc *p) {
	/* create thread for init */
	Thread *curThread = createInitial(p);
	setRunning(curThread);
	return curThread;
}

Thread *ThreadBase::createInitial(Proc *p) {
	Thread *t = (Thread*)Cache::alloc(sizeof(Thread));
	if(t == NULL)
		Util::panic("Unable to allocate mem for initial thread");

	t->tid = nextTid++;
	t->proc = p;
	t->flags = 0;
	t->initProps();
	t->state = Thread::RUNNING;
	for(size_t i = 0; i < STACK_REG_COUNT; i++)
		t->stackRegions[i] = NULL;
	t->tlsRegion = NULL;
	if(initArch(t) < 0)
		Util::panic("Unable to init the arch-specific attributes of initial thread");

	/* create list */
	if(!t->add())
		Util::panic("Unable to put initial thread into the thread-list");

	/* insert in VFS; thread needs to be inserted for it */
	if(!VFS::createThread(t->getTid()))
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
	sigHandler = NULL;
	currentSignal = 0;
	deliveredSignal = 0;
	pending.count = 0;
	pending.first = NULL;
	pending.last = NULL;
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
	reqFrames = ISList<frameno_t>();
}

size_t ThreadBase::getCount() {
	SpinLock::acquire(&lock);
	size_t len = threads.length();
	SpinLock::release(&lock);
	return len;
}

int ThreadBase::extendStack(uintptr_t address) {
	Thread *t = Thread::getRunning();
	int res = 0;
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		/* if it does not yet exist, report an error */
		if(t->stackRegions[i] == NULL)
			return -ENOMEM;

		res = t->getProc()->getVM()->growStackTo(t->stackRegions[i],address);
		if(res >= 0)
			return res;
	}
	return res;
}

bool ThreadBase::getStackRange(uintptr_t *start,uintptr_t *end,size_t stackNo) const {
	bool res = false;
	if(stackRegions[stackNo] != NULL)
		res = proc->getVM()->getRegRange(stackRegions[stackNo],start,end,false);
	return res;
}

bool ThreadBase::getTLSRange(uintptr_t *start,uintptr_t *end) const {
	bool res = false;
	if(tlsRegion != NULL)
		res = proc->getVM()->getRegRange(tlsRegion,start,end,false);
	return res;
}

void ThreadBase::updateRuntimes() {
	uint64_t cyclesPerSec = Thread::ticksPerSec();
	SpinLock::acquire(&lock);
	size_t threadCount = threads.length();
	for(auto t = threads.begin(); t != threads.end(); ++t) {
		/* update cycle-stats */
		if((*t)->state == Thread::RUNNING) {
			/* we want to measure the last second only */
			uint64_t cycles = Thread::getTSC() - (*t)->stats.cycleStart;
			(*t)->stats.lastCycleCount = (*t)->stats.curCycleCount + MIN(cyclesPerSec,cycles);
		}
		else
			(*t)->stats.lastCycleCount = (*t)->stats.curCycleCount;
		(*t)->stats.curCycleCount = 0;

		/* raise/lower the priority if necessary */
		Sched::adjustPrio(*t,threadCount);
	}
	SpinLock::release(&lock);
}

bool ThreadBase::reserveFrames(size_t count) {
	while(count > 0) {
		if(!PhysMem::reserve(count))
			return false;
		for(size_t i = count; i > 0; i--) {
			frameno_t frm = PhysMem::allocate(PhysMem::USR);
			if(!frm)
				break;
			reqFrames.append(frm);
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
		for(size_t i = 0; i < STACK_REG_COUNT; i++) {
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
	if(!VFS::createThread(t->getTid()))
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
	assert(state == Thread::ZOMBIE);
	/* remove tls */
	if(tlsRegion != NULL) {
		proc->getVM()->remove(tlsRegion);
		tlsRegion = NULL;
	}

	/* release resources */
	for(size_t i = 0; i < termLockCount; i++)
		SpinLock::release(termLocks[i]);
	for(size_t i = 0; i < termHeapCount; i++)
		Cache::free(termHeapAllocs[i]);
	for(size_t i = 0; i < termUsageCount; i++)
		termUsages[i]->decUsages();
	for(size_t i = 0; i < termCallbackCount; i++)
		termCallbacks[i]();

	/* remove from all modules we may be announced */
	makeUnrunnable();
	Timer::removeThread(tid);
	Signals::removeHandlerFor(tid);
	freeArch(static_cast<Thread*>(this));
	VFS::removeThread(tid);

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

void ThreadBase::printAll(OStream &os) {
	for(auto t = threads.cbegin(); t != threads.cend(); ++t)
		(*t)->print(os);
}

static const char *getStateName(uint8_t state) {
	static const char *states[] = {
		"UNUSED","RUN","RDY","BLK","ZOM","BLKSWAP","RDYSWAP"
	};
	return states[state];
}

void ThreadBase::printShort(OStream &os) const {
	os.writef("%d [state=%s, prio=%d, cpu=%d, time=%Lums, ev=",
			tid,getStateName(state),priority,cpu,getRuntime());
	Event::printEvMask(os,static_cast<const Thread*>(this));
	os.writef("]");
}

void ThreadBase::print(OStream &os) const {
	os.writef("Thread %d: (process %d:%s)\n",tid,proc->getPid(),proc->getCommand());
	os.pushIndent();
	os.writef("Flags=%#x\n",flags);
	os.writef("State=%s\n",getStateName(state));
	os.writef("Events=");
	Event::printEvMask(os,static_cast<const Thread*>(this));
	os.writef("\n");
	os.writef("LastCPU=%d\n",cpu);
	os.writef("TlsRegion=%p, ",tlsRegion ? tlsRegion->virt : 0);
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		os.writef("stackRegion%zu=%p",i,stackRegions[i] ? stackRegions[i]->virt : 0);
		if(i < STACK_REG_COUNT - 1)
			os.writef(", ");
	}
	os.writef("\n");
	os.writef("Priority = %d\n",priority);
	os.writef("Runtime = %Lums\n",getRuntime());
	os.writef("Scheduled = %u\n",stats.schedCount);
	os.writef("Syscalls = %u\n",stats.syscalls);
	os.writef("CurCycleCount = %Lu\n",stats.curCycleCount);
	os.writef("LastCycleCount = %Lu\n",stats.lastCycleCount);
	os.writef("cycleStart = %Lu\n",stats.cycleStart);
	os.writef("Kernel-trace:\n");
	os.pushIndent();
	{
		Util::FuncCall *calls = Util::getKernelStackTraceOf(static_cast<const Thread*>(this));
		while(calls->addr != 0) {
			os.writef("%p -> %p (%s)\n",(calls + 1)->addr,calls->funcAddr,calls->funcName);
			calls++;
		}
	}
	os.popIndent();
	{
		Util::FuncCall *calls = Util::getUserStackTraceOf(
				const_cast<Thread*>(static_cast<const Thread*>(this)));
		if(calls) {
			os.writef("User-trace:\n");
			os.pushIndent();
			while(calls->addr != 0) {
				os.writef("%p -> %p (%s)\n",
						(calls + 1)->addr,calls->funcAddr,calls->funcName);
				calls++;
			}
			os.popIndent();
		}
	}
	Util::printUserStateOf(os,static_cast<const Thread*>(this));
	os.popIndent();
}

tid_t ThreadBase::getFreeTid() {
	size_t count = 0;
	tid_t res = INVALID_TID;
	SpinLock::acquire(&lock);
	while(count < MAX_THREAD_COUNT) {
		if(nextTid >= MAX_THREAD_COUNT)
			nextTid = 0;
		if(tidToThread[nextTid++] == NULL) {
			res = nextTid - 1;
			break;
		}
		count++;
	}
	SpinLock::release(&lock);
	return res;
}

bool ThreadBase::add() {
	bool res = false;
	SpinLock::acquire(&lock);
	if(threads.append(static_cast<Thread*>(this))) {
		tidToThread[tid] = static_cast<Thread*>(this);
		res = true;
	}
	SpinLock::release(&lock);
	return res;
}

void ThreadBase::remove() {
	SpinLock::acquire(&lock);
	threads.remove(static_cast<Thread*>(this));
	tidToThread[tid] = NULL;
	SpinLock::release(&lock);
}
