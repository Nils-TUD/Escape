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

#define MAX_THREAD_NAME_LEN		15
#define MAX_STACK_PAGES			128

/* TODO move to types */
typedef u16 tTid;

/* the thread-state which will be saved for context-switching */
typedef struct {
	u32 esp;
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 eflags;
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
	sProc *process;
	/* thread id */
	tTid tid;
	/* start address of the stack */
	u32 stackBegin;
	u32 stackPages;
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
	/* thread-name */
	char name[MAX_THREAD_NAME_LEN + 1];
} sThread;

void thread_init(sProc *p);

sThread *thread_getRunning(void);

sThread *thread_getById(tTid tid);

sThread *thread_create(sProc *p);

s32 thread_clone(void);

s32 thread_destroy(void);

#endif /* THREAD_H_ */
