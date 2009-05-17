/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include "common.h"
#include "fpu.h"
#include "proc.h"

/* TODO we need the thread-count for sched, atm */
#define THREAD_COUNT			1024
#define MAX_THREAD_NAME_LEN		15
#define MAX_STACK_PAGES			128

#define INVALID_TID				THREAD_COUNT

/* the thread-state which will be saved for context-switching */
typedef struct {
	u32 esp;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 eflags;
	/* note that we don't need to save eip because when we're done in thread_resume() we have
	 * our kernel-stack back which causes the ret-instruction to return to the point where
	 * we've called thread_save(). */
} sThreadRegs;

/* the thread states */
/*typedef enum {ST_UNUSED = 0,ST_RUNNING = 1,ST_READY = 2,ST_BLOCKED = 3,ST_ZOMBIE = 4} eThreadState;*/

/* represents a thread */
typedef struct {
	/* thread state. see eThreadState */
	u8 state;
	/* the events the thread waits for (if waiting) */
	u8 events;
	/* the signal that the thread is currently handling (if > 0) */
	tSig signal;
	/* the process we belong to */
	sProc *proc;
	/* thread id */
	tTid tid;
	/* start address of the stack */
	u32 ustackBegin;
	u32 ustackPages;
	u32 kstackFrame;
	/* TODO just for debugging atm */
	u32 ueip;
	sThreadRegs save;
	/* FPU-state; initially NULL */
	sFPUState *fpuState;
	/* number of cpu-cycles the thread has used so far; TODO: should be cpu-time later */
	u64 ucycleStart;
	u64 ucycleCount;
	u64 kcycleStart;
	u64 kcycleCount;
	u64 cycleCount;
} sThread;

sThread *thread_init(sProc *p);

/**
 * Saves the state of the current thread in the given area
 *
 * @param saveArea the area where to save the state
 * @return false for the caller-thread, true for the resumed thread
 */
extern bool thread_save(sThreadRegs *saveArea);

/**
 * Resumes the given state
 *
 * @param pageDir the physical address of the page-dir
 * @param saveArea the area to load the state from
 * @param kstackFrame the frame-number of the kernel-stack
 * @return always true
 */
extern bool thread_resume(u32 pageDir,sThreadRegs *saveArea,u32 kstackFrame);

sThread *thread_getRunning(void);

sThread *thread_getById(tTid tid);

void thread_switch(void);

void thread_switchTo(tTid tid);

/**
 * Puts the given thraed to sleep with given wake-up-events
 *
 * @param tid the thread to put to sleep
 * @param events the events on which the thread should wakeup
 */
void thread_wait(tTid tid,u8 events);

/**
 * Wakes up all blocked threads that wait for the given event
 *
 * @param event the event
 */
void thread_wakeupAll(u8 event);

/**
 * Wakes up the given thread with the given event. If the thread is not waiting for it
 * the event is ignored
 *
 * @param tid the thread to wakeup
 * @param event the event to send
 */
void thread_wakeup(tTid tid,u8 event);

s32 thread_extendStack(u32 address);

s32 thread_clone(sThread *src,sThread **dst,u32 *stackFrame,bool cloneProc);

void thread_destroy(sThread *t,bool destroyStacks);


/* #### TEST/DEBUG FUNCTIONS #### */
#if DEBUGGING

void thread_dbg_printAll(void);

void thread_dbg_printShort(sThread *t);

#endif

#endif /* THREAD_H_ */
