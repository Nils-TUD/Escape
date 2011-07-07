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
#include <sys/task/event.h>
#include <sys/task/signals.h>
#include <sys/mem/paging.h>
#include <esc/hashmap.h>

#ifdef __i386__
#include <sys/arch/i586/task/thread.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/thread.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/thread.h>
#endif

#define MAX_STACK_PAGES			128

#define INITIAL_STACK_PAGES		1

#define INIT_TID				0
#define IDLE_TID				1
#define ATA_TID					3

#define MAX_THREAD_COUNT		8192
#define INVALID_TID				0xFFFF
/* use an invalid tid to identify the kernel */
#define KERNEL_TID				0xFFFE

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
	/* thread state. see eThreadState */
	uint8_t state;
	/* whether signals should be ignored (while being blocked) */
	uint8_t ignoreSignals;
	/* thread id */
	tid_t tid;
	/* the events the thread waits for (if waiting) */
	uint events;
	sWait *waits;
	/* the process we belong to */
	sProc *proc;
	/* the stack-region(s) for this thread */
	vmreg_t stackRegions[STACK_REG_COUNT];
	/* the TLS-region for this thread (-1 if not present) */
	vmreg_t tlsRegion;
	/* the frame mapped at KERNEL_STACK */
	frameno_t kstackFrame;
	/* pointer to the kernel-stack end when entering the kernel */
	sIntrptStackFrame *kstackEnd;
	/* the save-area for registers */
	sThreadRegs save;
	/* architecture-specific attributes */
	sThreadArchAttr archAttr;
	struct {
		/* number of cpu-cycles the thread has used so far */
		uint64_t ucycleStart;
		uLongLong ucycleCount;
		uint64_t kcycleStart;
		uLongLong kcycleCount;
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
 * Fetches the thread with given id from the internal thread-map
 *
 * @param tid the thread-id
 * @return the thread or NULL if not found
 */
sThread *thread_getById(tid_t tid);

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
 * Marks the given thread as ready (if the thread hasn't said that he don't wants to be interrupted)
 *
 * @param tid the thread-id
 * @return true if the thread is ready now
 */
bool thread_setReady(tid_t tid);

/**
 * Marks the given thread as blocked
 *
 * @param tid the thread-id
 * @return true if the thread is blocked now
 */
bool thread_setBlocked(tid_t tid);

/**
 * Marks the given thread as suspended or not-suspended
 *
 * @param tid the thread-id
 * @param blocked whether to suspend or "unsuspend" the thread
 */
void thread_setSuspended(tid_t tid,bool blocked);

/**
 * Checks whether the given thread has the given region-number for stack
 *
 * @param t the thread
 * @param regNo the region-number
 * @return true if so
 */
bool thread_hasStackRegion(const sThread *t,vmreg_t regNo);

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
 * Finishes the clone of a thread
 *
 * @param t the original thread
 * @param nt the new thread
 * @return 0 for the parent, 1 for the child
 */
int thread_finishClone(sThread *t,sThread *nt);

/**
 * Clones <src> to <dst>. That means a new thread will be created and <src> will be copied to the
 * new one.
 *
 * @param src the thread to copy
 * @param dst will contain a pointer to the new thread
 * @param p the process the thread should belong to
 * @param stackFrame will contain the stack-frame that has been used for the kernel-stack of the
 * 	new thread
 * @param cloneProc whether a process is cloned or just a thread
 * @return 0 on success
 */
int thread_clone(const sThread *src,sThread **dst,sProc *p,frameno_t *stackFrame,bool cloneProc);

/**
 * Clones the architecture-specific attributes of the given thread
 *
 * @param src the thread to copy
 * @param dst will contain a pointer to the new thread
 * @param cloneProc whether a process is cloned or just a thread
 * @return 0 on success
 */
int thread_cloneArch(const sThread *src,sThread *dst,bool cloneProc);

/**
 * Kills the given thread. If it is the current one it will be stored for later deletion.
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
