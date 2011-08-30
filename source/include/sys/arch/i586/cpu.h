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

#ifndef I586_CPU_H_
#define I586_CPU_H_

#include <sys/common.h>
#include <sys/printf.h>

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

typedef struct {
	uint8_t vendor;
	uint16_t model;
	uint16_t family;
	uint16_t type;
	uint16_t brand;
	uint16_t stepping;
	uint32_t signature;
	uint32_t features;
} sCPUInfo;

enum eCPUIdRequests {
	CPUID_GETVENDORSTRING,
	CPUID_GETFEATURES,
	CPUID_GETTLB,
	CPUID_GETSERIAL,

	CPUID_INTELEXTENDED=(int)0x80000000,
	CPUID_INTELFEATURES,
	CPUID_INTELBRANDSTRING,
	CPUID_INTELBRANDSTRINGMORE,
	CPUID_INTELBRANDSTRINGEND
};

/**
 * @return true if the cpuid-instruction is supported
 */
extern bool cpu_cpuidSupported(void);

/**
 * "Detects" the CPU
 */
void cpu_detect(void);

/**
 * @return whether the machine has a local APIC
 */
bool cpu_hasLocalAPIC(void);

/**
 * Issue a single request to CPUID
 *
 * @param code the request to perform
 * @param a will contain the value of eax
 * @param b will contain the value of ebx
 * @param c will contain the value of ecx
 * @param d will contain the value of edx
 */
extern void cpu_getInfo(uint32_t code,uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d);

/**
 * Issues the given request to CPUID and stores the result in <res>
 *
 * @param code the request to perform
 * @param res will contain the result
 */
extern void cpu_getStrInfo(uint32_t code,char *res);

/**
 * @return the value of the CR0 register
 */
extern uint32_t cpu_getCR0(void);

/**
 * @param cr0 the new CR0 value
 */
extern void cpu_setCR0(uint32_t cr0);

/**
 * @return the value of the CR2 register, that means the linear address that caused a page-fault
 */
extern uint32_t cpu_getCR2(void);

/**
 * @return the value of the CR3 register, that means the physical address of the page-directory
 */
extern uint32_t cpu_getCR3(void);

/**
 * @return the value of the CR4 register
 */
extern uint32_t cpu_getCR4(void);

/**
 * @param cr4 the new CR4 value
 */
extern void cpu_setCR4(uint32_t cr4);

/**
 * @param msr the msr
 * @return the value of the given model-specific-register
 */
extern uint64_t cpu_getMSR(uint32_t msr);

/**
 * @param msr the msr
 * @param value the new value
 */
extern void cpu_setMSR(uint32_t msr,uint64_t value);

#endif /*I586_CPU_H_*/
