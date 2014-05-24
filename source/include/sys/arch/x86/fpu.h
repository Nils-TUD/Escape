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

#include <sys/common.h>
#include <sys/cpu.h>

class FPU {
	FPU() = delete;

public:
	/* the state of FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 */
	struct XState {
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
	 * @param state pointer to the state-memory
	 */
	static void handleCoProcNA(XState **state);

	/**
	 * Clones the FPU-state <src> into <dst>
	 *
	 * @param dst the destination-fpu-state
	 * @param src the source-fpu-state
	 */
	static void cloneState(XState **dst,const XState *src);

	/**
	 * Free's the given FPU-state
	 *
	 * @param state the state
	 */
	static void freeState(XState **state);

private:
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
	static XState ***curStates;
};
