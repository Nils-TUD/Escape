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

#ifndef THREAD_H_
#define THREAD_H_

#include <sys/common.h>
#include <sys/task/proc.h>
#include <sys/task/signals.h>
#include <sys/mem/paging.h>
#include <sys/mem/vmreg.h>
#include <esc/hashmap.h>

#ifdef __i386__
#include <sys/arch/i586/task/threadconf.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/threadconf.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/threadconf.h>
#endif

#define MAX_INTRPT_LEVELS		3
#define MAX_STACK_PAGES			128
#define INITIAL_STACK_PAGES		1

#define MAX_PRIO				4
#define DEFAULT_PRIO			3
/* 1 means, 100% of the timeslice, 2 = 200% of the timeslice */
#define PRIO_BAD_SLICE_MULT		4
/* 1 means, 100% of the timeslice, 2 means 50% of the timeslice */
#define PRIO_GOOD_SLICE_DIV		2
/* number of times a good-ratio has to be reached in a row to raise the priority again */
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

#define T_IDLE					1

typedef void (*fTermCallback)(void);

typedef struct sWait {
	tid_t tid;
	ushort evi;
	evobj_t object;
	struct sWait *prev;
	struct sWait *next;
	struct sWait *tnext;
} sWait;

/* the thread states */
typedef enum {
	ST_UNUSED = 0,
	ST_RUNNING = 1,
	ST_READY = 2,
	ST_BLOCKED = 3,
	ST_ZOMBIE = 4,
	/* suspended => CAN'T run. will be set to blocked when resumed (also used for swapping) */
	ST_BLOCKED_SUSP = 5,
	/* same as ST_BLOCKED_SUSP, but will be set to ready when done */
	ST_READY_SUSP = 6,
	/* same as ST_BLOCKED_SUSP, but will be set to zombie when done */
	ST_ZOMBIE_SUSP = 7
} eThreadState;

/* represents a thread */
typedef struct sThread sThread;
struct sThread {
	const uint8_t flags;
	/* the thread-priority (0..MAX_PRIO) */
	uint8_t priority;
	/* a counter used to raise the priority after a certain number of "good behaviours" */
	uint8_t prioGoodCnt;
	/* the current thread state. see eThreadState */
	uint8_t state;
	/* the next state it will receive on context-switch */
	uint8_t newState;
	/* the current or last cpu that executed this thread */
	cpuid_t cpu;
	/* whether signals should be ignored (while being blocked) */
	uint8_t ignoreSignals;
	/* the signal-data, managed by the signals-module */
	sSignals *signals;
	/* thread id */
	const tid_t tid;
	/* the events the thread waits for (if waiting) */
	uint events;
	sWait *waits;
	/* the process we belong to */
	sProc *const proc;
	/* the stack-region(s) for this thread */
	sVMRegion *stackRegions[STACK_REG_COUNT];
	/* the TLS-region for this thread (-1 if not present) */
	sVMRegion *tlsRegion;
	/* stack of pointers to the end of the kernel-stack when entering kernel */
	sIntrptStackFrame *intrptLevels[MAX_INTRPT_LEVELS];
	size_t intrptLevel;
	/* the save-area for registers */
	sThreadRegs save;
	/* architecture-specific attributes */
	sThreadArchAttr archAttr;
	/* the number of allocated resources (thread can't die until resources=0) */
	uint8_t resources;
	/* a list with heap-allocations that should be free'd on thread-termination */
	void *termHeapAllocs[TERM_RESOURCE_CNT];
	/* a list of locks that should be released on thread-termination */
	klock_t *termLocks[TERM_RESOURCE_CNT];
	/* a list of file-usages that should be decremented on thread-termination */
	sFile *termUsages[TERM_RESOURCE_CNT];
	/* a list of callbacks that should be called on thread-termination */
	fTermCallback termCallbacks[TERM_RESOURCE_CNT];
	uint8_t termHeapCount;
	uint8_t termLockCount;
	uint8_t termUsageCount;
	uint8_t termCallbackCount;
	/* a list of currently requested frames, i.e. frames that are not free anymore, but were
	 * reserved for this thread and have not yet been used */
	sSLList reqFrames;
	struct {
		uint64_t timeslice;
		/* number of microseconds of runtime this thread has got so far */
		uint64_t runtime;
		/* executed cycles in this second */
		uint64_t curCycleCount;
		uint64_t cycleStart;
		/* executed cycles in the previous second */
		uint64_t lastCycleCount;
		/* the number of times we got chosen so far */
		ulong syscalls;
		ulong schedCount;
	} stats;
	/* for the scheduler */
	sThread *prev;
	sThread *next;
};

#ifdef __i386__
#include <sys/arch/i586/task/thread.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/thread.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/thread.h>
#endif

/**
 * The start-function for the idle-thread
 */
extern void thread_idle(void);

/**
 * Inits the threading-stuff. Uses <p> as first process
 *
 * @param p the first process
 * @return the first thread
 */
sThread *thread_init(sProc *p);

/**
 * Inits the architecture-specific attributes of the given thread
 */
int thread_initArch(sThread *t);

/**
 * Adds a stack to the given initial thread. This is just used for initloader.
 *
 * @param t the thread
 */
void thread_addInitialStack(sThread *t);

/**
 * @return the current interrupt-stack, i.e. the innermost-level
 *
 * @param t the thread
 */
sIntrptStackFrame *thread_getIntrptStack(const sThread *t);

/**
 * Pushes the given kernel-stack onto the interrupt-level-stack
 *
 * @param t the thread
 * @param stack the kernel-stack
 * @return the current level
 */
size_t thread_pushIntrptLevel(sThread *t,sIntrptStackFrame *stack);

/**
 * Removes the topmost interrupt-level-stack
 *
 * @param t the thread
 */
void thread_popIntrptLevel(sThread *t);

/**
 * @param t the thread
 * @return the interrupt-level (0 .. MAX_INTRPT_LEVEL - 1)
 */
size_t thread_getIntrptLevel(const sThread *t);

/**
 * @return the number of existing threads
 */
size_t thread_getCount(void);

/**
 * @return the currently running thread
 */
sThread *thread_getRunning(void);

/**
 * Sets the currently running thread. Should ONLY be called by thread_switchTo()!
 *
 * @param t the thread
 */
void thread_setRunning(sThread *t);

/**
 * Checks whether the thread can be terminated. If so, it ensures that it won't be scheduled again.
 *
 * @param t the thread
 * @return true if it can be terminated now
 */
bool thread_beginTerm(sThread *t);

/**
 * Fetches the thread with given id from the internal thread-map
 *
 * @param tid the thread-id
 * @return the thread or NULL if not found
 */
sThread *thread_getById(tid_t tid);

/**
 * Pushes the given thread back to the idle-list
 *
 * @param t the idle-thread
 */
void thread_pushIdle(sThread *t);

/**
 * Pops an idle-thread from the idle-list
 *
 * @return the thread
 */
sThread *thread_popIdle(void);

/**
 * Retrieves the range of the stack with given number.
 *
 * @param t the thread
 * @param start will be set to the start-address (may be NULL)
 * @param end will be set to the end-address (may be NULL)
 * @param stackNo the stack-number
 * @return true if the stack-region exists
 */
bool thread_getStackRange(sThread *t,uintptr_t *start,uintptr_t *end,size_t stackNo);

/**
 * Retrieves the range of the TLS region
 *
 * @param t the thread
 * @param start will be set to the start-address (may be NULL)
 * @param end will be set to the end-address (may be NULL)
 * @return true if the TLS-region exists
 */
bool thread_getTLSRange(sThread *t,uintptr_t *start,uintptr_t *end);

/**
 * @param t the thread
 * @return the region of the given thread (NULL if not existing)
 */
sVMRegion *thread_getTLSRegion(const sThread *t);

/**
 * Sets the TLS-region for the given thread
 *
 * @param t the thread
 * @param vm the region
 */
void thread_setTLSRegion(sThread *t,sVMRegion *vm);

/**
 * Performs a thread-switch. That means the current thread will be saved and the first thread
 * will be picked from the ready-queue and resumed.
 */
void thread_switch(void);

/**
 * Switches the thread and remembers that we are waiting in the kernel. That means if you want
 * to wait in the kernel for something, use this function so that other modules know that the
 * current thread waits and should for example not receive signals.
 */
void thread_switchNoSigs(void);

/**
 * Switches to the given thread
 *
 * @param tid the thread-id
 */
void thread_switchTo(tid_t tid);

/**
 * Kills dead threads, if there are any; is called by thread_switchTo(). Should NOT be called by
 * anyone else.
 */
void thread_killDead(void);

/**
 * Blocks the given thread. ONLY CALLED by event.
 *
 * @param t the thread
 */
void thread_block(sThread *t);

/**
 * Unblocks the given thread. ONLY CALLED by event.
 *
 * @param t the thread
 */
void thread_unblock(sThread *t);

/**
 * Unblocks the given thread and puts it to the beginning of the ready-list. ONLY CALLED by event.
 *
 * @param t the thread
 */
void thread_unblockQuick(sThread *t);

/**
 * Suspends the given thread. ONLY CALLED by event.
 *
 * @param t the thread
 */
void thread_suspend(sThread *t);

/**
 * Resumes the given thread. ONLY CALLED by event.
 *
 * @param t the thread
 */
void thread_unsuspend(sThread *t);

/**
 * Checks whether the given thread has the given region for stack
 *
 * @param t the thread
 * @param vm the region
 * @return true if so
 */
bool thread_hasStackRegion(const sThread *t,sVMRegion *vm);

/**
 * Removes the regions for this thread. Optionally the stack as well.
 *
 * @param t the thread
 * @param remStack whether to remove the stack
 */
void thread_removeRegions(sThread *t,bool remStack);

/**
 * Extends the stack of the current thread so that the given address is accessible. If that
 * is not possible the function returns a negative error-code
 *
 * IMPORTANT: May cause a thread-switch for swapping!
 *
 * @param address the address that should be accessible
 * @return 0 on success
 */
int thread_extendStack(uintptr_t address);

/**
 * @param thread the thread
 * @return the runtime of the given thread
 */
uint64_t thread_getRuntime(const sThread *t);

/**
 * @param thread the thread
 * @return the cycles of the given thread
 */
uint64_t thread_getCycles(const sThread *t);

/**
 * Updates the runtime-percentage of all threads
 */
void thread_updateRuntimes(void);

/**
 * Reserves <count> frames for the current thread. That means, it swaps in memory, if
 * necessary, allocates that frames and stores them in the thread. You can get them later with
 * thread_getFrame(). You can free not needed frames with thread_discardFrames().
 *
 * @param count the number of frames to reserve
 * @return true on success
 */
bool thread_reserveFrames(size_t count);

/**
 * The same as thread_reserveFrames(), but does it for <t> instead of for the running thread.
 *
 * @param t the thread to reserve the frames for
 * @param count the number of frames to reserve
 * @return true on success
 */
bool thread_reserveFramesFor(sThread *t,size_t count);

/**
 * Removes one frame from the collection of frames of the current thread. This will always succeed,
 * because the function assumes that you have called thread_reserveFrames() previously.
 *
 * @return the frame
 */
frameno_t thread_getFrame(void);

/**
 * Free's all frames that the current thread has still reserved. This should be done after an
 * operation that needed more frames to ensure that reserved but not needed frames are free'd.
 */
void thread_discardFrames(void);

/**
 * The same as thread_discardFrames(), but does it for <t> instead of for the running thread.
 *
 * @param t the thread to discard the frames for
 */
void thread_discardFramesFor(sThread *t);

/**
 * Adds the given lock to the term-lock-list
 *
 * @param l the lock
 */
void thread_addLock(klock_t *l);

/**
 * Removes the given lock from the term-lock-list
 *
 * @param l the lock
 */
void thread_remLock(klock_t *l);

/**
 * Adds the given pointer to the term-heap-allocation-list, which will be free'd if the thread dies
 * before it is removed.
 *
 * @param ptr the pointer to the heap
 */
void thread_addHeapAlloc(void *ptr);

/**
 * Removes the given pointer from the term-heap-allocation-list
 *
 * @param ptr the pointer to the heap
 */
void thread_remHeapAlloc(void *ptr);

/**
 * Adds the given file to the file-usage-list. The file-usages will be decreased if the thread dies.
 *
 * @param file the file
 */
void thread_addFileUsage(sFile *file);

/**
 * Removes the given file from the file-usage-list.
 *
 * @param file the file
 */
void thread_remFileUsage(sFile *file);

/**
 * Adds the given callback to the callback-list. These will be called on thread-termination.
 *
 * @param callback the callback
 */
void thread_addCallback(void (*callback)(void));

/**
 * Removes the given callback from the callback-list.
 *
 * @param callback the callback
 */
void thread_remCallback(void (*callback)(void));

/**
 * Finishes the clone of a thread
 *
 * @param t the original thread
 * @param nt the new thread
 * @return 0 for the parent, 1 for the child
 */
int thread_finishClone(sThread *t,sThread *nt);

/**
 * Performs the finish-operations after the thread nt has been started, but before it runs.
 *
 * @param t the running thread
 * @param tn the started thread
 * @param arg the argument
 * @param entryPoint the entry-point of the thread (in kernel or in user-space)
 */
void thread_finishThreadStart(sThread *t,sThread *nt,const void *arg,uintptr_t entryPoint);

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
int thread_create(sThread *src,sThread **dst,sProc *p,uint8_t flags,bool cloneProc);

/**
 * Determines the number of necessary frames to start a new thread
 */
size_t thread_getThreadFrmCnt(void);

/**
 * Clones the architecture-specific attributes of the given thread
 *
 * @param src the thread to copy
 * @param dst will contain a pointer to the new thread
 * @param cloneProc whether a process is cloned or just a thread
 * @return 0 on success
 */
int thread_createArch(const sThread *src,sThread *dst,bool cloneProc);

/**
 * Terminates the given thread, i.e. adds it to the terminator and if its the running thread, it
 * makes sure that it isn't chosen again.
 *
 * @param t the thread to terminate
 */
void thread_terminate(sThread *t);

/**
 * Kills the given thread, i.e. releases all resources and destroys it.
 *
 * @param t the thread
 */
void thread_kill(sThread *t);

/**
 * Frees the architecture-specific attributes of the given thread
 *
 * @param t the thread
 */
void thread_freeArch(sThread *t);

/**
 * Prints all threads
 */
void thread_printAll(void);

/**
 * Prints a short info about the given thread
 *
 * @param t the thread
 */
void thread_printShort(const sThread *t);

/**
 * Prints the given thread
 *
 * @param t the thread
 */
void thread_print(const sThread *t);

/**
 * Prints the given thread-state
 *
 * @param state the pointer to the state-struct
 */
void thread_printState(const sThreadRegs *state);

#endif /* THREAD_H_ */
