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

#pragma once

#include <sys/common.h>
#include <sys/task/signals.h>
#include <sys/task/sched.h>
#include <sys/task/terminator.h>
#include <sys/task/timer.h>
#include <sys/mem/pagedir.h>
#include <sys/mem/vmtree.h>
#include <sys/mem/virtmem.h>
#include <sys/col/dlist.h>
#include <sys/interrupts.h>
#include <esc/hashmap.h>
#include <assert.h>

#define MAX_INTRPT_LEVELS		3
#define MAX_STACK_PAGES			128
#define INITIAL_STACK_PAGES		1

#define MAX_PRIO				4
/* if a thread was blocked less than BAD_BLOCKED_TIME(t), the priority is lowered */
#define BAD_BLOCK_TIME(total)	((total) / 6)
/* if a thread was blocked more than GOOD_BLOCKED_TIME(t), the priority is raised again */
#define GOOD_BLOCK_TIME(total)	((total) / 2)
/* number of times a good-blocked-time has to be reached in a row to raise the priority again */
#define PRIO_FORGIVE_CNT		8

/* reset the runtime and update priorities every 1sec */
#define RUNTIME_UPDATE_INTVAL	1000

#define INIT_TID				0
#define ATA_TID					3

#define MAX_THREAD_COUNT		8192
#define INVALID_TID				0xFFFF
/* use an invalid tid to identify the kernel */
#define KERNEL_TID				0xFFFE

#define TERM_RESOURCE_CNT		4
#define TERM_USAGE_CNT			2
#define TERM_CALLBACK_CNT		1

#define T_IDLE					1
#define T_WILL_DIE				2

#ifdef __i386__
#include <sys/arch/i586/task/threadconf.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/threadconf.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/threadconf.h>
#endif

typedef void (*terminate_func)();

class Thread;
class Proc;
class Event;

class ThreadBase : public DListItem {
	friend class ProcBase;
	friend class Sched;
	friend class Signals;
	friend class Event;

	struct Stats {
		/* number of microseconds of runtime this thread has got so far */
		uint64_t runtime;
		/* executed cycles in this second */
		uint64_t curCycleCount;
		uint64_t cycleStart;
		/* executed cycles in the previous second */
		uint64_t lastCycleCount;
		/* the cycles we were blocked */
		uint64_t blocked;
		/* syscall count and scheduling count */
		ulong syscalls;
		ulong schedCount;
		ulong migrations;
	};

public:
	struct ListItem : public DListItem {
		explicit ListItem(Thread *t) : DListItem(), thread(t) {
		}
		Thread *thread;
	};

	/* the thread states */
	enum State {
		RUNNING,
		READY,
		BLOCKED,
		ZOMBIE,
		/* suspended => CAN'T run. will be set to blocked when resumed (also used for swapping) */
		BLOCKED_SUSP,
		/* same as ST_BLOCKED_SUSP, but will be set to ready when done */
		READY_SUSP,
		/* same as ST_BLOCKED_SUSP, but will be set to zombie when done */
		ZOMBIE_SUSP
	};

	ThreadBase() = delete;

	/**
	 * Determines the number of necessary frames to start a new thread
	 */
	static size_t getThreadFrmCnt();

	/**
	 * @return the number of existing threads
	 */
	static size_t getCount() {
		return threads.length();
	}

	/**
	 * @return the currently running thread
	 */
	static Thread *getRunning();

	/**
	 * Sets the currently running thread. Should ONLY be called by switchTo()!
	 */
	static void setRunning(Thread *t);

	/**
	 * Fetches the thread with given id from the internal thread-map
	 *
	 * @param tid the thread-id
	 * @return the thread or NULL if not found
	 */
	static Thread *getById(tid_t tid);

	/**
	 * Checks whether the given thread exists and adds a reference if so.
	 *
	 * @param tid the tid of the thread
	 * @return if it exists, the thread
	 */
	static Thread *getRef(tid_t tid);

	/**
	 * Releases the reference of given thread that has been added previously by getRef(). If there
	 * are no further references, the thread is destroyed.
	 *
	 * @param t the thread
	 */
	static void relRef(const Thread *t);

	/**
	 * Pushes the given thread back to the idle-list
	 *
	 * @param t the idle-thread
	 */
	static void pushIdle(Thread *t);

	/**
	 * Pops an idle-thread from the idle-list
	 *
	 * @return the thread
	 */
	static Thread *popIdle();

	/**
	 * Performs a thread-switch. That means the current thread will be saved and the first thread
	 * will be picked from the ready-queue and resumed.
	 */
	static void switchAway() {
		doSwitch();
	}

	/**
	 * Switches the thread and remembers that we are waiting in the kernel. That means if you want
	 * to wait in the kernel for something, use this function so that other modules know that the
	 * current thread waits and should for example not receive signals.
	 */
	static void switchNoSigs();

	/**
	 * Switches to the given thread
	 *
	 * @param tid the thread-id
	 */
	static void switchTo(tid_t tid);

	/**
	 * Extends the stack of the current thread so that the given address is accessible. If that
	 * is not possible the function returns a negative error-code
	 *
	 * IMPORTANT: May cause a thread-switch for swapping!
	 *
	 * @param address the address that should be accessible
	 * @return 0 on success
	 */
	static int extendStack(uintptr_t address);

	/**
	 * Updates the runtime-percentage of all threads
	 */
	static void updateRuntimes();

	/**
	 * Adds the given lock to the term-lock-list
	 *
	 * @param l the lock
	 */
	static void addLock(klock_t *l);

	/**
	 * Removes the given lock from the term-lock-list
	 *
	 * @param l the lock
	 */
	static void remLock(klock_t *l);

	/**
	 * Adds the given pointer to the term-heap-allocation-list, which will be free'd if the thread dies
	 * before it is removed.
	 *
	 * @param ptr the pointer to the heap
	 */
	static void addHeapAlloc(void *ptr);

	/**
	 * Removes the given pointer from the term-heap-allocation-list
	 *
	 * @param ptr the pointer to the heap
	 */
	static void remHeapAlloc(void *ptr);

	/**
	 * Adds the given file to the file-usage-list. The file-usages will be decreased if the thread dies.
	 *
	 * @param file the file
	 */
	static void addFileUsage(OpenFile *file);

	/**
	 * Removes the given file from the file-usage-list.
	 *
	 * @param file the file
	 */
	static void remFileUsage(OpenFile *file);

	/**
	 * Adds the given callback to the callback-list. These will be called on thread-termination.
	 *
	 * @param callback the callback
	 */
	static void addCallback(void (*callback)());

	/**
	 * Removes the given callback from the callback-list.
	 *
	 * @param callback the callback
	 */
	static void remCallback(void (*callback)());

	/**
	 * Prints all threads
	 *
	 * @param os the output-stream
	 */
	static void printAll(OStream &os);


	/**
	 * Adds a stack to the given initial thread. This is just used for initloader.
	 *
	 * @param t the thread
	 */
	void addInitialStack();

	/**
	 * @return the thread-id
	 */
	tid_t getTid() const {
		return tid;
	}

	/**
	 * @return the process the thread belongs to
	 */
	Proc *getProc() const {
		return proc;
	}

	/**
	 * @return the flags of this thread (T_*)
	 */
	uint8_t getFlags() const {
		return flags;
	}
	void setFlags(uint8_t flags) {
		this->flags = flags;
	}
	/**
	 * @return the priority of this thread (0..MAX_PRIO)
	 */
	uint8_t getPriority() const {
		return priority;
	}
	void setPriority(uint8_t prio) {
		assert(prio <= MAX_PRIO);
		priority = prio;
	}
	/**
	 * @return the state of this thread (ST_*)
	 */
	uint8_t getState() const {
		return state;
	}
	void setState(uint8_t state) {
		this->state = state;
	}
	/**
	 * @return the next state a thread should receive if we schedule away from him
	 */
	uint8_t getNewState() const {
		return newState;
	}

	/**
	 * @return the current or last cpu that executed this thread
	 */
	cpuid_t getCPU() const {
		return cpu;
	}
	void setCPU(cpuid_t cpu) {
		this->cpu = cpu;
	}

	/**
	 * @return the region of this thread (NULL if not existing)
	 */
	VMRegion *getTLSRegion() const {
		return tlsRegion;
	}
	void setTLSRegion(VMRegion *vm) {
		tlsRegion = vm;
	}

	/**
	 * @return the stack region with given number
	 */
	VMRegion *getStackRegion(size_t no) const {
		return stackRegions[no];
	}

	/**
	 * @return true if the thread ignores signals currently
	 */
	bool isIgnoringSigs() const {
		return ignoreSignals;
	}

	/**
	 * @return true if this thread has active resources
	 */
	bool hasResources() const {
		return resources > 0;
	}
	/**
	 * Increments the resource-counter
	 */
	void addResource() {
		resources++;
	}
	/**
	 * Decrements the resource-counter
	 */
	void remResource() {
		assert(resources > 0);
		resources--;
	}

	/**
	 * @param thread the thread
	 * @return the runtime of the given thread
	 */
	uint64_t getRuntime() const {
		return Timer::cyclesToTime(stats.runtime);
	}

	/**
	 * Tests whether there are other threads with a higher priority than this one.
	 *
	 * @return true if so
	 */
	bool haveHigherPrio() {
		ulong mask = Sched::getReadyMask();
		return mask & ~((1UL << (priority + 1)) - 1);
	}

	/**
	 * @param thread the thread
	 * @return the cycles of the given thread
	 */
	Stats &getStats() {
		return stats;
	}
	const Stats &getStats() const {
		return stats;
	}

	/**
	 * @return the inode for the thread-directory in the VFS
	 */
	inode_t getThreadDir() const {
		return threadDir;
	}

	/**
	 * This is the quick way of checking for signals. Since this is done without locking, it is
	 * possible that in this moment a signal is added and we miss it. But this isn't bad because
	 * we'll simply handle it later, as we would have anyway if it had arrived a bit later.
	 * It might also mean that there actually is no signal to handle because we're currently handling
	 * one.
	 *
	 * @return whether there is a signal to handle
	 */
	bool hasSignalQuick() const {
		return pending.count > 0;
	}

	/**
	 * @return the register state of the thread
	 */
	const ThreadRegs &getRegs() const {
		return saveArea;
	}

	/**
	 * @return the current interrupt-stack, i.e. the innermost-level
	 */
	IntrptStackFrame *getIntrptStack() const;

	/**
	 * Pushes the given kernel-stack onto the interrupt-level-stack
	 *
	 * @param stack the kernel-stack
	 * @return the current level
	 */
	size_t pushIntrptLevel(IntrptStackFrame *stack);

	/**
	 * Removes the topmost interrupt-level-stack
	 */
	void popIntrptLevel();

	/**
	 * @param t the thread
	 * @return the interrupt-level (0 .. MAX_INTRPT_LEVEL - 1)
	 */
	size_t getIntrptLevel() const;

	/**
	 * Checks whether the thread can be terminated. If so, it ensures that it won't be scheduled again.
	 *
	 * @return true if it can be terminated now
	 */
	bool beginTerm();

	/**
	 * Retrieves the range of the stack with given number.
	 *
	 * @param start will be set to the start-address (may be NULL)
	 * @param end will be set to the end-address (may be NULL)
	 * @param stackNo the stack-number
	 * @return true if the stack-region exists
	 */
	bool getStackRange(uintptr_t *start,uintptr_t *end,size_t stackNo) const;

	/**
	 * Retrieves the range of the TLS region
	 *
	 * @param start will be set to the start-address (may be NULL)
	 * @param end will be set to the end-address (may be NULL)
	 * @return true if the TLS-region exists
	 */
	bool getTLSRange(uintptr_t *start,uintptr_t *end) const;

	/**
	 * Lets this thread wait for the given event and object
	 *
	 * @param event the event to wait for
	 * @param object the object (0 = ignore)
	 * @return true if successfull
	 */
	void wait(uint event,evobj_t object);

	/**
	 * Blocks this thread. ONLY CALLED by event.
	 */
	void block();

	/**
	 * Unblocks this thread. ONLY CALLED by event.
	 */
	void unblock();

	/**
	 * Unblocks this thread and puts it to the beginning of the ready-list. ONLY CALLED by event.
	 */
	void unblockQuick();

	/**
	 * Suspends this thread. ONLY CALLED by event.
	 */
	void suspend();

	/**
	 * Resumes this thread. ONLY CALLED by event.
	 */
	void unsuspend();

	/**
	 * Checks whether this thread has the given region for stack
	 *
	 * @param vm the region
	 * @return true if so
	 */
	bool hasStackRegion(VMRegion *vm) const;

	/**
	 * Removes the regions for this thread. Optionally the stack as well.
	 *
	 * @param remStack whether to remove the stack
	 */
	void removeRegions(bool remStack);

	/**
	 * @return the number of reserved frames
	 */
	size_t getReservedFrmCnt() const {
		return reqFrames.length();
	}

	/**
	 * Reserves <count> frames for this thread. That means, it swaps in memory, if
	 * necessary and <swap> is true, allocates that frames and stores them in the thread.
	 * You can get them later with getFrame(). You can free not needed frames with discardFrames().
	 *
	 * @param count the number of frames to reserve
	 * @param swap whether to swap or just return false if there is not enough
	 * @return true on success
	 */
	bool reserveFrames(size_t count,bool swap = true);

	/**
	 * Removes one frame from the collection of frames of this thread. This will always succeed,
	 * because the function assumes that you have called reserveFrames() previously.
	 *
	 * @return the frame
	 */
	frameno_t getFrame();

	/**
	 * Free's all frames that this thread has still reserved. This should be done after an
	 * operation that needed more frames to ensure that reserved but not needed frames are free'd.
	 */
	void discardFrames();

	/**
	 * Prints a short info about this thread
	 *
	 * @param os the output-stream
	 */
	void printShort(OStream &os) const;

	/**
	 * Prints this thread
	 *
	 * @param os the output-stream
	 */
	void print(OStream &os) const;

	/**
	 * Prints the given thread-state
	 *
	 * @param os the output-stream
	 * @param st the pointer to the state-struct
	 */
	void printState(OStream &os,const ThreadRegs *st) const;

private:
	/**
	 * Inits the threading-stuff. Uses <p> as first process
	 *
	 * @param p the first process
	 * @return the first thread
	 */
	static Thread *init(Proc *p);

	/**
	 * Inits the architecture-specific attributes of the given thread
	 */
	static int initArch(Thread *t);

	/**
	 * Clones <src> to <dst>. That means a new thread will be created and <src> will be copied to the
	 * new one.
	 *
	 * @param src the thread to copy
	 * @param dst will contain a pointer to the new thread
	 * @param p the process the thread should belong to
	 * @param flags the flags for the thread (T_*)
	 * @param cloneProc whether a process is cloned or just a thread
	 * @return 0 on success
	 */
	static int create(Thread *src,Thread **dst,Proc *p,uint8_t flags,bool cloneProc);

	/**
	 * Finishes the clone of a thread
	 *
	 * @param t the original thread
	 * @param nt the new thread
	 * @return 0 for the parent, 1 for the child
	 */
	static int finishClone(Thread *t,Thread *nt);

	/**
	 * Performs the finish-operations after the thread nt has been started, but before it runs.
	 *
	 * @param t the running thread
	 * @param tn the started thread
	 * @param arg the argument
	 * @param entryPoint the entry-point of the thread (in kernel or in user-space)
	 */
	static void finishThreadStart(Thread *t,Thread *nt,const void *arg,uintptr_t entryPoint);

	/**
	 * Clones the architecture-specific attributes of the given thread
	 *
	 * @param src the thread to copy
	 * @param dst will contain a pointer to the new thread
	 * @param cloneProc whether a process is cloned or just a thread
	 * @return 0 on success
	 */
	static int createArch(const Thread *src,Thread *dst,bool cloneProc);

	/**
	 * Frees the architecture-specific attributes of the given thread
	 */
	static void freeArch(Thread *t);

	/**
	 * Terminates this thread, i.e. adds it to the terminator and if its the running thread, it
	 * makes sure that it isn't chosen again.
	 */
	void terminate();

	/**
	 * Kills this thread, i.e. releases all resources and destroys it.
	 */
	void kill();

	void setNewState(uint8_t state) {
		newState = state;
	}

	void printEvMask(OStream &os) const;
	void makeUnrunnable();
	void initProps();
	static void doSwitch() asm("thread_switch");
	static Thread *createInitial(Proc *p);
	static tid_t getFreeTid();
	void add();
	void remove();

protected:
	/* thread id */
	tid_t tid;
	/* the number of references to this thread */
	mutable ushort refs;
	/* the process we belong to */
	Proc *proc;
	/* the signal-data, managed by the signals-module */
	Signals::handler_func *sigHandler;
	/* list of pending signals */
	Signals::PendingQueue pending;
	/* the signal that the thread is currently handling (if > 0) */
	int currentSignal;
	/* the event the thread waits for (if waiting) */
	uint event;
	evobj_t evobject;
	uint64_t waitstart;
	/* a counter used to raise the priority after a certain number of "good behaviours" */
	uint8_t prioGoodCnt;
	uint8_t flags;
	uint8_t priority;
	uint8_t state;
	/* the next state it will receive on context-switch */
	uint8_t newState;
	cpuid_t cpu;
	/* whether signals should be ignored (while being blocked) */
	uint8_t ignoreSignals;
	/* the number of allocated resources (thread can't die until resources=0) */
	uint8_t resources;
	/* the stack-region(s) for this thread */
	VMRegion *stackRegions[STACK_REG_COUNT];
	/* the TLS-region for this thread (-1 if not present) */
	VMRegion *tlsRegion;
	/* thread-directory in VFS */
	inode_t threadDir;
	/* stack of pointers to the end of the kernel-stack when entering kernel */
	IntrptStackFrame *intrptLevels[MAX_INTRPT_LEVELS];
	size_t intrptLevel;
	/* the save-area for registers */
	ThreadRegs saveArea;
	/* a list with heap-allocations that should be free'd on thread-termination */
	void *termHeapAllocs[TERM_RESOURCE_CNT];
	/* a list of locks that should be released on thread-termination */
	klock_t *termLocks[TERM_RESOURCE_CNT];
	/* a list of file-usages that should be decremented on thread-termination */
	OpenFile *termUsages[TERM_USAGE_CNT];
	/* a list of callbacks that should be called on thread-termination */
	terminate_func termCallbacks[TERM_CALLBACK_CNT];
	uint8_t termHeapCount;
	uint8_t termLockCount;
	uint8_t termUsageCount;
	uint8_t termCallbackCount;
	ListItem threadListItem;
	ListItem signalListItem;
	/* a list of currently requested frames, i.e. frames that are not free anymore, but were
	 * reserved for this thread and have not yet been used */
	ISList<frameno_t> reqFrames;
	Stats stats;

private:
	static DList<ListItem> threads;
	static Thread *tidToThread[MAX_THREAD_COUNT];
	static tid_t nextTid;
	static klock_t refLock;
	static Mutex mutex;
};

#ifdef __i386__
#include <sys/arch/i586/task/thread.h>
static_assert(sizeof(Thread) <= 256,"Thread is too big");
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/thread.h>
// TODO actually, there is not much missing for 256 bytes
static_assert(sizeof(Thread) <= 512,"Thread is too big");
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/thread.h>
// TODO actually, there is not much missing for 512 bytes
static_assert(sizeof(Thread) <= 1024,"Thread is too big");
#endif

inline Thread *ThreadBase::getById(tid_t tid) {
	if(tid >= ARRAY_SIZE(tidToThread))
		return NULL;
	return tidToThread[tid];
}

inline void ThreadBase::switchNoSigs() {
	ThreadBase *t = getRunning();
	/* remember that the current thread wants to ignore signals */
	t->ignoreSignals = 1;
	switchAway();
	t->ignoreSignals = 0;
}

inline void ThreadBase::addLock(klock_t *l) {
	Thread *cur = getRunning();
	assert(cur->termLockCount < TERM_RESOURCE_CNT);
	cur->termLocks[cur->termLockCount++] = l;
}

inline void ThreadBase::remLock(klock_t *l) {
	Thread *cur = getRunning();
	assert(cur->termLockCount > 0 && cur->termLocks[cur->termLockCount - 1] == l);
	cur->termLockCount--;
}

inline void ThreadBase::addHeapAlloc(void *ptr) {
	Thread *cur = getRunning();
	assert(cur->termHeapCount < TERM_RESOURCE_CNT);
	cur->termHeapAllocs[cur->termHeapCount++] = ptr;
}

inline void ThreadBase::remHeapAlloc(void *ptr) {
	Thread *cur = getRunning();
	assert(cur->termHeapCount > 0 && cur->termHeapAllocs[cur->termHeapCount - 1] == ptr);
	cur->termHeapCount--;
}

inline void ThreadBase::addFileUsage(OpenFile *file) {
	Thread *cur = getRunning();
	assert(cur->termUsageCount < TERM_USAGE_CNT);
	cur->termUsages[cur->termUsageCount++] = file;
}

inline void ThreadBase::remFileUsage(OpenFile *file) {
	Thread *cur = getRunning();
	assert(cur->termUsageCount > 0 && cur->termUsages[cur->termUsageCount - 1] == file);
	cur->termUsageCount--;
}

inline void ThreadBase::addCallback(void (*callback)()) {
	Thread *cur = getRunning();
	assert(cur->termCallbackCount < TERM_CALLBACK_CNT);
	cur->termCallbacks[cur->termCallbackCount++] = callback;
}

inline void ThreadBase::remCallback(void (*callback)()) {
	Thread *cur = getRunning();
	assert(cur->termCallbackCount > 0 && cur->termCallbacks[cur->termCallbackCount - 1] == callback);
	cur->termCallbackCount--;
}

inline IntrptStackFrame *ThreadBase::getIntrptStack() const {
	if(intrptLevel > 0)
		return intrptLevels[intrptLevel - 1];
	return NULL;
}

inline size_t ThreadBase::pushIntrptLevel(IntrptStackFrame *stack) {
	assert(intrptLevel < MAX_INTRPT_LEVELS);
	intrptLevels[intrptLevel++] = stack;
	return intrptLevel;
}

inline void ThreadBase::popIntrptLevel() {
	assert(intrptLevel > 0);
	intrptLevel--;
}

inline size_t ThreadBase::getIntrptLevel() const {
	assert(intrptLevel > 0);
	return intrptLevel - 1;
}

inline void ThreadBase::wait(uint event,evobj_t object) {
	assert(this != NULL);
	Sched::wait(static_cast<Thread*>(this),event,object);
}

inline void ThreadBase::block() {
	assert(this != NULL);
	Sched::block(static_cast<Thread*>(this));
}

inline void ThreadBase::unblock() {
	assert(this != NULL);
	Sched::unblock(static_cast<Thread*>(this));
}

inline void ThreadBase::unblockQuick() {
	Sched::unblockQuick(static_cast<Thread*>(this));
}

inline void ThreadBase::suspend() {
	assert(this != NULL);
	Sched::suspend(static_cast<Thread*>(this));
}

inline void ThreadBase::unsuspend() {
	assert(this != NULL);
	Sched::unsuspend(static_cast<Thread*>(this));
}

inline bool ThreadBase::hasStackRegion(VMRegion *vm) const {
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		if(stackRegions[i] == vm)
			return true;
	}
	return false;
}

inline void ThreadBase::removeRegions(bool remStack) {
	tlsRegion = NULL;
	if(remStack) {
		for(size_t i = 0; i < STACK_REG_COUNT; i++)
			stackRegions[i] = NULL;
	}
	/* remove all signal-handler since we've removed the code to handle signals */
	Signals::removeHandlerFor(tid);
}

inline frameno_t ThreadBase::getFrame() {
	frameno_t frm = reqFrames.removeFirst();
	assert(frm != 0);
	return frm;
}

inline void ThreadBase::discardFrames() {
	frameno_t frm;
	while((frm = reqFrames.removeFirst()) != 0)
		PhysMem::free(frm,PhysMem::USR);
}

inline void ThreadBase::terminate() {
	/* if its the current one, it can't be chosen again by the scheduler */
	if(this == getRunning())
		makeUnrunnable();
	Terminator::addDead(static_cast<Thread*>(this));
}

/**
 * The start-function for the idle-thread
 */
EXTERN_C void thread_idle();
