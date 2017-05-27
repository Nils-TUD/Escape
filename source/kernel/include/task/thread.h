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

#pragma once

#include <esc/col/dlist.h>
#include <mem/pagedir.h>
#include <mem/virtmem.h>
#include <mem/vmtree.h>
#include <task/sched.h>
#include <task/signals.h>
#include <task/timer.h>
#include <assert.h>
#include <common.h>
#include <interrupts.h>

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

#define MAX_THREAD_COUNT		8192
#define INVALID_TID				0xFFFF
/* use an invalid tid to identify the kernel */
#define KERNEL_TID				0xFFFE

#define T_IDLE					1
#define T_IGNSIGS				2
#define T_FPU_WAIT				4
#define T_FPU_SIGNAL			8

#if defined(__i586__)
#	include <arch/i586/task/threadconf.h>
#elif defined(__x86_64__)
#	include <arch/x86_64/task/threadconf.h>
#elif defined(__eco32__)
#	include <arch/eco32/task/threadconf.h>
#elif defined(__mmix__)
#	include <arch/mmix/task/threadconf.h>
#endif

class Thread;
class Proc;
class Event;

class ThreadBase : public esc::DListItem {
	friend class ProcBase;
	friend class Sched;
	friend class Signals;
	friend class Event;
	friend class Terminator;

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
		/* the signal that killed the thread (SIG_COUNT if none) */
		int signal;
		/* exit-code the thread gave us via exit() */
		int exitCode;
	};

protected:
	explicit ThreadBase(Proc *p,uint8_t flags);

public:
	struct ListItem : public esc::DListItem {
		explicit ListItem(Thread *t) : esc::DListItem(), thread(t) {
		}
		Thread *thread;
	};

	/* the thread states */
	enum State {
		RUNNING,
		READY,
		BLOCKED,
		ZOMBIE
	};

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
	 * Prints all threads
	 *
	 * @param os the output-stream
	 */
	static void printAll(OStream &os);


	/**
	 * Adds a stack to the given initial thread. This is just used for initloader.
	 *
	 * @param t the thread
	 * @return the stack address
	 */
	uintptr_t addInitialStack();

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
	 * @return the stack region with given number
	 */
	VMRegion *getStackRegion(size_t no) const {
		return stackRegions[no];
	}

	/**
	 * @return true if the thread ignores signals currently
	 */
	bool isIgnoringSigs() const {
		return flags & T_IGNSIGS;
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
	ino_t getThreadDir() const {
		return threadDir;
	}

	/**
	 * @return whether there is a signal to handle
	 */
	bool hasSignal() const {
		return !isIgnoringSigs() && sigmask != 0;
	}

	/**
	 * @return whether this thread has a handler for the given signal
	 */
	bool hasSigHandler(int signal) const {
		return sigHandler[signal] != SIG_DFL;
	}

	/**
	 * @return true if the thread has got a segfault signal
	 */
	bool isFaulted() const {
		volatile uint32_t *mask = const_cast<volatile uint32_t*>(&sigmask);
		return (*mask & (1 << SIGSEGV)) != 0;
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
	IntrptStackFrame *getUserState() const;

	/**
	 * @param tid the thread id
	 * @return true if the given thread belongs to the same process as this one
	 */
	bool isSameProcess(tid_t tid);

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
	 * Terminates the thread, i.e. releases most of the resources and announces itself to the
	 * terminator, which will do the rest.
	 */
	void terminate();

	/**
	 * Finally kills this thread, i.e. releases the last resources and notifies the process.
	 */
	void kill();

	void setNewState(uint8_t state) {
		newState = state;
	}

	void printEvMask(OStream &os) const;
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
	Signals::handler_func sigHandler[SIG_COUNT];
	/* mask of pending signals */
	uint32_t sigmask;
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
	/* the stack-region(s) for this thread */
	VMRegion *stackRegions[STACK_REG_COUNT];
	/* thread-directory in VFS */
	ino_t threadDir;
	/* the save-area for registers */
	ThreadRegs saveArea;
	ListItem threadListItem;
	ListItem signalListItem;
	/* a list of currently requested frames, i.e. frames that are not free anymore, but were
	 * reserved for this thread and have not yet been used */
	esc::ISList<frameno_t> reqFrames;
	Stats stats;

private:
	static esc::DList<ListItem> threads;
	static Thread *tidToThread[MAX_THREAD_COUNT];
	static tid_t nextTid;
	static SpinLock refLock;
	static Mutex mutex;
};

#if defined(__i586__)
#	include <arch/x86/task/thread.h>
static_assert(sizeof(Thread) <= 256,"Thread is too big");
#elif defined(__x86_64__)
#	include <arch/x86/task/thread.h>
static_assert(sizeof(Thread) <= 512,"Thread is too big");
#elif defined(__eco32__)
#	include <arch/eco32/task/thread.h>
// TODO actually, there is not much missing for 256 bytes
static_assert(sizeof(Thread) <= 512,"Thread is too big");
#elif defined(__mmix__)
#	include <arch/mmix/task/thread.h>
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
	t->flags |= T_IGNSIGS;
	switchAway();
	t->flags &= ~T_IGNSIGS;
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

inline bool ThreadBase::hasStackRegion(VMRegion *vm) const {
	for(size_t i = 0; i < STACK_REG_COUNT; i++) {
		if(stackRegions[i] == vm)
			return true;
	}
	return false;
}

inline void ThreadBase::removeRegions(bool remStack) {
	if(remStack) {
		for(size_t i = 0; i < STACK_REG_COUNT; i++)
			stackRegions[i] = NULL;
	}
	/* remove all signal-handler since we've removed the code to handle signals */
	memset(sigHandler,0,sizeof(sigHandler));
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

/**
 * The start-function for the idle-thread
 */
EXTERN_C void thread_idle();
