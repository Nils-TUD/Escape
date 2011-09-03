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
#include <esc/hashmap.h>

#define MAX_INTRPT_LEVELS		3
#define MAX_STACK_PAGES			128

#define INITIAL_STACK_PAGES		1

#define INIT_TID				0
#define ATA_TID					3

#define MAX_THREAD_COUNT		8192
#define INVALID_TID				0xFFFF
/* use an invalid tid to identify the kernel */
#define KERNEL_TID				0xFFFE

#define T_IDLE					1

#ifdef __i386__
#include <sys/arch/i586/task/thread.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/thread.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/thread.h>
#endif

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
	/* the current thread state. see eThreadState */
	uint8_t state;
	/* the next state it will receive on context-switch */
	uint8_t newState;
	/* to lock the attributes of the thread */
	klock_t lock;
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
	vmreg_t stackRegions[STACK_REG_COUNT];
	/* the TLS-region for this thread (-1 if not present) */
	vmreg_t tlsRegion;
	/* stack of pointers to the end of the kernel-stack when entering kernel */
	sIntrptStackFrame *intrptLevels[MAX_INTRPT_LEVELS];
	size_t intrptLevel;
	/* the save-area for registers */
	sThreadRegs save;
	/* architecture-specific attributes */
	sThreadArchAttr archAttr;
	/* a list with heap-allocations that should be free'd on thread-termination */
	sSLList termHeapAllocs;
	/* a list of locks that should be released on thread-termination */
	sSLList termLocks;
	/* a list of file-usages that should be decremented on thread-termination */
	sSLList termUsages;
	struct {
		/* number of microseconds of runtime this thread has got so far */
		uint64_t runtime;
		/* executed cycles in this second */
		uint64_t curCycleCount;
		uint64_t cycleStart;
		/* executed cycles in the previous second */
		uint64_t lastCycleCount;
		/* the number of times we got chosen so far */
		ulong schedCount;
		ulong syscalls;
	} stats;
	/* for the scheduler */
	sThread *prev;
	sThread *next;
};

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
 * Returns whether the given thread is currently running in a safe way, i.e. it will only be
 * reported that its not running, if the thread-switch is completely finished.
 *
 * @param t the thread
 * @return true if its running
 */
bool thread_isRunning(sThread *t);

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
 * @return the TLS-region number of the given thread (-1 if not existing)
 */
vmreg_t thread_getTLSRegion(const sThread *t);

/**
 * Sets the TLS-region for the given thread
 *
 * @param t the thread
 * @param rno the region-number
 */
void thread_setTLSRegion(sThread *t,vmreg_t rno);

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
 * Checks whether the given thread has the given region-number for stack
 *
 * @param t the thread
 * @param regNo the region-number
 * @return true if so
 */
bool thread_hasStackRegion(const sThread *t,vmreg_t regNo);

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
void thread_addFileUsage(file_t file);

/**
 * Removes the given file from the file-usage-list.
 *
 * @param file the file
 */
void thread_remFileUsage(file_t file);

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
 * Clones the architecture-specific attributes of the given thread
 *
 * @param src the thread to copy
 * @param dst will contain a pointer to the new thread
 * @param cloneProc whether a process is cloned or just a thread
 * @return 0 on success
 */
int thread_createArch(const sThread *src,sThread *dst,bool cloneProc);

/**
 * Terminates the given thread. That means, it removes it from scheduler and adds it to the
 * termination-module.
 *
 * @param t the thread (may be the current one or run on a different cpu)
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
