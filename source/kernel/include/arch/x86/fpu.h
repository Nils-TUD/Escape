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

#include <common.h>
#include <cpu.h>
#include <semaphore.h>
#include <spinlock.h>

class Thread;

class FPU {
	FPU() = delete;

public:
	/* the state of FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 */
	struct XState : public CacheAllocatable {
		uint8_t bytes[512];
	} A_PACKED;

	/**
	 * Inits the FPU for usage of processes
	 */
	static void init();

	/**
	 * Locks the FPU so that we'll receive a EX_CO_PROC_NA exception as soon as someone
	 * uses a FPU-instruction
	 */
	static void lockFPU() {
		/* set the task-switched-bit in CR0. as soon as a process uses any FPU instruction
		 * we'll get a EX_CO_PROC_NA and call handleCoProcNA() */
		ulong cr0 = CPU::getCR0();
		if(~cr0 & CPU::CR0_TASK_SWITCHED)
			CPU::setCR0(cr0 | CPU::CR0_TASK_SWITCHED);
	}

	/**
	 * Handles the EX_CO_PROC_NA exception
	 *
	 * @param t the current thread
	 */
	static void handleCoProcNA(Thread *t);

	/**
	 * Clones the FPU-state from thread <src> into <dst>.
	 *
	 * @param dst the destination thread
	 * @param src the source thread
	 */
	static void cloneState(Thread *dst,const Thread *src);

	/**
	 * Initiates a saving of the FPU state on the old CPU of the given thread.
	 *
	 * @param t the current thread
	 */
	static void initSaveState(Thread *t);

	/**
	 * Saves the FPU-state to the current state owner.
	 *
	 * @param t the current thread
	 */
	static void saveState(Thread *t);

	/**
	 * Free's the FPU-state of the given thread.
	 *
	 * @param thread the thread
	 */
	static void freeState(Thread *t);

private:
	static bool doSave(XState **current);

	static void finit() {
		asm volatile ("fninit");
	}
	static void fsave(XState *state) {
		asm volatile ("fxsave %0" : : "m"(*state));
	}
	static void frestore(XState *state) {
		asm volatile ("fxrstor %0" : : "m"(*state));
	}

	/* current FPU state-memory */
	static Thread **curStates;
	static SpinLock lock;
};
