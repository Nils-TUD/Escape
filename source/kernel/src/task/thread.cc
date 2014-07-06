/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <sys/task/timer.h>
#include <sys/task/terminator.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/openfile.h>
#include <sys/mem/cache.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/physmem.h>
#include <sys/mem/virtmem.h>
#include <sys/task/sched.h>
#include <sys/task/smp.h>
#include <sys/cpu.h>
#include <sys/log.h>
#include <sys/spinlock.h>
#include <sys/util.h>
#include <sys/video.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

/* our threads */
DList<Thread::ListItem> ThreadBase::threads;
Thread *ThreadBase::tidToThread[MAX_THREAD_COUNT];
tid_t ThreadBase::nextTid = 0;
SpinLock ThreadBase::refLock;
Mutex ThreadBase::mutex;

Thread *ThreadBase::getRef(tid_t tid) {
	if(tid >= ARRAY_SIZE(tidToThread))
		return NULL;

	LockGuard<SpinLock> g(&refLock);
	Thread *t = tidToThread[tid];
	if(t)
		t->refs++;
	return t;
}

void ThreadBase::relRef(const Thread *t) {
	LockGuard<SpinLock> g(&refLock);
	if(--t->refs == 0) {
		Proc::relRef(t->proc);
		const_cast<Thread*>(t)->remove();
		Cache::free(const_cast<Thread*>(t));
	}
}

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
	t->priority = MAX_PRIO;
	for(size_t i = 0; i < STACK_REG_COUNT; i++)
		t->stackRegions[i] = NULL;
	if(initArch(t) < 0)
		Util::panic("Unable to init the arch-specific attributes of initial thread");

	t->add();

	/* insert in VFS; thread needs to be inserted for it */
	t->threadDir = VFS::createThread(t->getTid());
	if(t->threadDir < 0)
		Util::panic("Unable to put first thread in vfs");
	return t;
}

void ThreadBase::initProps() {
	state = Thread::BLOCKED;
	newState = Thread::READY;
	prioGoodCnt = 0;
	event = 0;
	evobject = 0;
	waitstart = 0;
	memset(sigHandler,0,sizeof(sigHandler));
	sigmask = 0;
	intrptLevel = 0;
	threadDir = 0;
	cpu = 0;
	stats.runtime = 0;
	stats.curCycleCount = 0;
	stats.lastCycleCount = 0;
	stats.cycleStart = CPU::rdtsc();
	stats.blocked = 0;
	stats.schedCount = 0;
	stats.syscalls = 0;
	stats.migrations = 0;
	reqFrames = ISList<frameno_t>();
	threadListItem = ListItem(static_cast<Thread*>(this));
	signalListItem = ListItem(static_cast<Thread*>(this));
	refs = 1;
}

int ThreadBase::extendStack(uintptr_t address) {
	Thread *t = Thread::getRunning();
	int res = 0;
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		/* if it does not yet exist, report an error */
		if(t->stackRegions[i] == NULL)
			return -EFAULT;

		res = t->getProc()->getVM()->growStackTo(t->stackRegions[i],address);
		if(res >= 0)
			return res;
	}
	return res;
}

bool ThreadBase::getStackRange(uintptr_t *start,uintptr_t *end,size_t stackNo) const {
	bool res = false;
	if(stackRegions[stackNo] != NULL) {
		proc->getVM()->getRegRange(stackRegions[stackNo],start,end,false);
		res = true;
	}
	return res;
}

void ThreadBase::updateRuntimes() {
	uint64_t cyclesPerSec = CPU::getSpeed();
	LockGuard<Mutex> g(&mutex);
	/* first store the max priority for all processes (this is no problem, because we're only using
	 * this as the prio for new threads while holding the same lock, see add()) */
	for(auto t = threads.begin(); t != threads.end(); ++t) {
		t->thread->proc->priority = MAX_PRIO;
		t->thread->proc->stats.lastCycles = 0;
	}

	for(auto it = threads.begin(); it != threads.end(); ++it) {
		Thread *t = it->thread;
		Proc *p = t->getProc();

		/* update cycle-stats */
		if(t->state == Thread::RUNNING) {
			/* we want to measure the last second only */
			uint64_t cycles = CPU::rdtsc() - t->stats.cycleStart;
			t->stats.lastCycleCount = t->stats.curCycleCount + MIN(cyclesPerSec,cycles);
		}
		else
			t->stats.lastCycleCount = t->stats.curCycleCount;
		p->getStats().totalRuntime += t->stats.lastCycleCount;
		p->getStats().lastCycles += t->stats.lastCycleCount;
		t->stats.curCycleCount = 0;

		/* don't adjust priority of idle-threads */
		if(~t->getFlags() & T_IDLE) {
			/* lower/raise priority of thread, if necessary */
			Sched::adjustPrio(t,cyclesPerSec);

			/* the process should always receive the minimum of all its thread priorities */
			if(t->priority < p->priority)
				p->priority = t->priority;
		}
	}
}

bool ThreadBase::reserveFrames(size_t count,bool swap) {
	while(count > 0) {
		if(!PhysMem::reserve(count,swap)) {
			discardFrames();
			return false;
		}
		for(size_t i = count; i > 0; i--) {
			frameno_t frm = PhysMem::allocate(PhysMem::USR);
			if(frm == INVALID_FRAME)
				break;
			reqFrames.append(frm);
			count--;
		}
	}
	return true;
}

int ThreadBase::create(Thread *src,Thread **dst,Proc *p,uint8_t tflags,bool cloneProc) {
	int err = -ENOMEM;
	Thread *t = (Thread*)Cache::alloc(sizeof(Thread));
	if(t == NULL)
		return -ENOMEM;

	t->proc = p;
	t->flags = tflags;
	t->initProps();

	/* determine tid (ensure that nobody else gets the same) and insert into thread-list */
	{
		LockGuard<Mutex> g(&mutex);
		t->tid = getFreeTid();
		if(t->tid == INVALID_TID) {
			Cache::free(t);
			return -ENOTHREADS;
		}
		t->add();
		/* do that here to prevent that one see's a temporary priority, i.e. during the update-phase */
		t->priority = p->getPriority();
	}

	/* we don't want to destroy the process first because we have a pointer to it */
	Proc::getRef(p->getPid());

	if(cloneProc) {
		for(size_t i = 0; i < STACK_REG_COUNT; i++) {
			if(src->stackRegions[i])
				t->stackRegions[i] = p->getVM()->getRegion(src->stackRegions[i]->virt());
			else
				t->stackRegions[i] = NULL;
		}
		t->intrptLevel = src->intrptLevel;
		memcpy(t->intrptLevels,src->intrptLevels,sizeof(IntrptStackFrame*) * MAX_INTRPT_LEVELS);
	}

	/* clone architecture-specific stuff */
	if((err = createArch(src,t,cloneProc)) < 0)
		goto errClone;


	/* append to idle-list if its an idle-thread */
	if(tflags & T_IDLE)
		Sched::addIdleThread(t);

	/* clone signal-handler (here because the thread needs to be in the map first) */
	if(cloneProc)
		memcpy(t->sigHandler,src->sigHandler,sizeof(src->sigHandler));

	/* insert in VFS; thread needs to be inserted for it */
	t->threadDir = VFS::createThread(t->getTid());
	if(t->threadDir < 0) {
		err = -ENOMEM;
		goto errAppendIdle;
	}

	*dst = t;
	return 0;

errAppendIdle:
	freeArch(t);
errClone:
	relRef(t);
	return err;
}

void ThreadBase::terminate() {
	assert(this == Thread::getRunning());

	/* process stats */
	proc->stats.totalRuntime += stats.curCycleCount;
	proc->stats.lastCycles += stats.curCycleCount;
	proc->stats.totalSyscalls += stats.syscalls;
	proc->stats.totalScheds += stats.schedCount;
	proc->stats.totalMigrations += stats.migrations;

	/* remove from all modules we may be announced */
	Sched::removeThread(static_cast<Thread*>(this));
	Timer::removeThread(tid);
	VFS::removeThread(tid);
	threadDir = -1;

	Terminator::addDead(static_cast<Thread*>(this));
}

void ThreadBase::kill() {
	assert(this != Thread::getRunning());
	freeArch(static_cast<Thread*>(this));

	VFS::removeThread(tid);

	/* notify the process about it */
	Sched::wakeup(EV_THREAD_DIED,(evobj_t)proc);

	/* unref and release. if there is nobody else, we'll destroy everything */
	relRef((Thread*)this);
}

void ThreadBase::printAll(OStream &os) {
	for(auto t = threads.cbegin(); t != threads.cend(); ++t)
		t->thread->print(os);
}

static const char *getStateName(uint8_t state) {
	static const char *states[] = {
		"RUN","RDY","BLK","ZOM","BLKSWAP","RDYSWAP"
	};
	return states[state];
}

void ThreadBase::printEvMask(OStream &os) const {
	if(event == 0)
		os.writef("-");
	else
		os.writef("%s:%p",Sched::getEventName(event),evobject);
}

void ThreadBase::printShort(OStream &os) const {
	os.writef("%d [state=%s, prio=%d, cpu=%d, time=%Lums, ev=",
			tid,getStateName(state),priority,cpu,getRuntime());
	this->printEvMask(os);
	os.writef("]");
}

void ThreadBase::print(OStream &os) const {
	os.writef("Thread %d: (process %d:%s)\n",tid,proc->getPid(),proc->getCommand());
	os.pushIndent();
	os.writef("References = %d\n",refs);
	os.writef("Flags = %#x\n",flags);
	os.writef("State = %s\n",getStateName(state));
	os.writef("Events = ");
	this->printEvMask(os);
	os.writef("\n");
	Signals::print(static_cast<const Thread*>(this),os);
	os.writef("LastCPU = %d\n",cpu);
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		os.writef("stackRegion%zu = %p",i,stackRegions[i] ? stackRegions[i]->virt() : 0);
		if(i + 1 < STACK_REG_COUNT)
			os.writef(", ");
	}
	os.writef("\n");
	os.writef("Priority = %d\n",priority);
	os.writef("Runtime = %Luus\n",getRuntime());
	os.writef("Blocked = %Lu\n",stats.blocked);
	os.writef("Scheduled = %lu\n",stats.schedCount);
	os.writef("Syscalls = %lu\n",stats.syscalls);
	os.writef("Migrations = %lu\n",stats.migrations);
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
	while(count < MAX_THREAD_COUNT) {
		if(nextTid >= MAX_THREAD_COUNT)
			nextTid = 0;
		if(tidToThread[nextTid++] == NULL) {
			res = nextTid - 1;
			break;
		}
		count++;
	}
	return res;
}

void ThreadBase::add() {
	threads.append(&threadListItem);
	tidToThread[tid] = static_cast<Thread*>(this);
}

void ThreadBase::remove() {
	LockGuard<Mutex> g(&mutex);
	threads.remove(&threadListItem);
	tidToThread[tid] = NULL;
}
