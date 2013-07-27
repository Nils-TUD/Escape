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

#pragma once

#include <sys/common.h>
#include <sys/printf.h>

class CPU : public CPUBase {
	friend class CPUBase;

	CPU() = delete;

	static const uint32_t FEATURE_LAPIC				= 1 << 9;
	static const size_t VENDOR_STRLEN				= 12;

	enum CPUIdRequests {
		CPUID_GETVENDORSTRING,
		CPUID_GETFEATURES,
		CPUID_GETTLB,
		CPUID_GETSERIAL,

		CPUID_INTELEXTENDED = (int)0x80000000,
		CPUID_INTELFEATURES,
		CPUID_INTELBRANDSTRING,
		CPUID_INTELBRANDSTRINGMORE,
		CPUID_INTELBRANDSTRINGEND
	};

public:
	static const uint32_t CR0_PAGING_ENABLED		= 1 << 31;
	/* Determines whether the CPU can write to pages marked read-only */
	static const uint32_t CR0_WRITE_PROTECT			= 1 << 16;
	/* Enables the native (internal) mechanism for reporting x87 FPU errors when set;
	 * enables the PC-style x87 FPU error reporting mechanism when clear. */
	static const uint32_t CR0_NUMERIC_ERROR			= 1 << 5;
	/* Allows the saving of the x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 context on a task switch
	 * to be delayed until an x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction is actually executed
	 * by the new task. The processor sets this flag on every task switch and tests it when executing
	 * x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
	static const uint32_t CR0_TASK_SWITCHED			= 1 << 3;
	/* Indicates that the processor does not have an internal or external x87 FPU when set; indicates
	 * an x87 FPU is present when clear. This flag also affects the execution of
	 * MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
	static const uint32_t CR0_EMULATE				= 1 << 2;
	/* Controls the interaction of the WAIT (or FWAIT) instruction with the TS flag (bit 3 of CR0).
	 * If the MP flag is set, a WAIT instruction generates a device-not-available exception (#NM) if
	 * the TS flag is also set. If the MP flag is clear, the WAIT instruction ignores the
	 * setting of the TS flag. */
	static const uint32_t CR0_MONITOR_COPROC		= 1 << 1;

	struct Info {
		uint8_t vendor;
		uint16_t model;
		uint16_t family;
		uint16_t type;
		uint16_t brand;
		uint16_t stepping;
		uint32_t signature;
		uint32_t features;
		uint32_t name[12];
	};

	/**
	 * @return true if the cpuid-instruction is supported
	 */
	static bool cpuidSupported() asm("cpu_cpuidSupported");

	/**
	 * "Detects" the CPU
	 */
	static void detect();

	/**
	 * @return whether the machine has a local APIC
	 */
	static bool hasLocalAPIC();

	/**
	 * Issue a single request to CPUID
	 *
	 * @param code the request to perform
	 * @param a will contain the value of eax
	 * @param b will contain the value of ebx
	 * @param c will contain the value of ecx
	 * @param d will contain the value of edx
	 */
	static void getInfo(uint32_t code,uint32_t *a,uint32_t *b,uint32_t *c,uint32_t *d) asm("cpu_getInfo");

	/**
	 * Issues the given request to CPUID and stores the result in <res>
	 *
	 * @param code the request to perform
	 * @param res will contain the result
	 */
	static void getStrInfo(uint32_t code,char *res) asm("cpu_getStrInfo");

	/**
	 * @return the value of the CR0 register
	 */
	static uint32_t getCR0() {
		uint32_t res;
		asm volatile ("mov %%cr0, %0" : "=r"(res));
		return res;
	}

	/**
	 * @param cr0 the new CR0 value
	 */
	static void setCR0(uint32_t cr0) {
		asm volatile ("mov %0, %%cr0" : : "r"(cr0));
	}

	/**
	 * @return the value of the CR2 register, that means the linear address that caused a page-fault
	 */
	static uint32_t getCR2() {
		uint32_t res;
		asm volatile ("mov %%cr2, %0" : "=r"(res));
		return res;
	}

	/**
	 * @return the value of the CR3 register, that means the physical address of the page-directory
	 */
	static uint32_t getCR3() {
		uint32_t res;
		asm volatile ("mov %%cr3, %0" : "=r"(res));
		return res;
	}

	/**
	 * @param cr0 the new CR3 value
	 */
	static void setCR3(uint32_t cr0) {
		asm volatile ("mov %0, %%cr3" : : "r"(cr0));
	}

	/**
	 * @return the value of the CR4 register
	 */
	static uint32_t getCR4() {
		uint32_t res;
		asm volatile ("mov %%cr4, %0" : "=r"(res));
		return res;
	}

	/**
	 * @param cr4 the new CR4 value
	 */
	static void setCR4(uint32_t cr4) {
		asm volatile ("mov %0, %%cr4" : : "r"(cr4));
	}

	/**
	 * @param msr the msr
	 * @return the value of the given model-specific-register
	 */
	static uint64_t getMSR(uint32_t msr) {
		uint32_t h,l;
        asm volatile (
        	"rdmsr" : "=a"(l), "=d"(h) : "c"(msr)
        );
        return (static_cast<uint64_t>(h) << 32) | l;
	}

	/**
	 * @param msr the msr
	 * @param value the new value
	 */
	static void setMSR(uint32_t msr,uint64_t value) {
		asm volatile (
			"wrmsr" : : "a"(static_cast<uint32_t>(value)),
						"d"(static_cast<uint32_t>(value >> 32)),
						"c"(msr)
		);
    }

private:
	/* the information about our cpu */
	static Info *cpus;
	static uint64_t cpuHz;
};

inline uint64_t CPUBase::getSpeed() {
	return CPU::cpuHz;
}

inline uint64_t CPUBase::rdtsc() {
	uint32_t u, l;
	asm volatile ("rdtsc" : "=a" (l), "=d" (u));
	return (uint64_t)u << 32 | l;
}
