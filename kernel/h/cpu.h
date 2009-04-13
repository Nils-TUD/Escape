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

#ifndef CPU_H_
#define CPU_H_

/* Enables the native (internal) mechanism for reporting x87 FPU errors when set;
 * enables the PC-style x87 FPU error reporting mechanism when clear. */
#define CR0_NUMERIC_ERROR			(1 << 5)

/* Allows the saving of the x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 context on a task switch
 * to be delayed until an x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction is actually executed
 * by the new task. The processor sets this flag on every task switch and tests it when executing
 * x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
#define CR0_TASK_SWITCHED			(1 << 3)

/* Indicates that the processor does not have an internal or external x87 FPU when set; indicates
 * an x87 FPU is present when clear. This flag also affects the execution of
 * MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
#define CR0_EMULATE					(1 << 2)

/* Controls the interaction of the WAIT (or FWAIT) instruction with the TS flag (bit 3 of CR0).
 * If the MP flag is set, a WAIT instruction generates a device-not-available exception (#NM) if
 * the TS flag is also set. If the MP flag is clear, the WAIT instruction ignores the
 * setting of the TS flag. */
#define CR0_MONITOR_COPROC			(1 << 1)

/**
 * @return the timestamp-counter value
 */
extern u64 cpu_rdtsc(void);

/**
 * @return the value of the CR0 register
 */
extern u32 cpu_getCR0(void);

/**
 * @param cr0 the new CR0 value
 */
extern void cpu_setCR0(u32 cr0);

/**
 * @return the value of the CR2 register, that means the linear address that caused a page-fault
 */
extern u32 cpu_getCR2(void);

/**
 * @return the value of the CR3 register, that means the physical address of the page-directory
 */
extern u32 cpu_getCR3(void);

/**
 * @return the value of the CR4 register
 */
extern u32 cpu_getCR4(void);

/**
 * @param cr4 the new CR4 value
 */
extern void cpu_setCR4(u32 cr4);

#endif /*CPU_H_*/
