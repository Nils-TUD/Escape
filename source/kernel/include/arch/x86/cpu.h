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

#include <sys/arch.h>
#include <common.h>

class CPU : public CPUBase {
	friend class CPUBase;

	CPU() = delete;

	enum CPUIdRequests {
		CPUID_GETFEATURES	= 1,
		CPUID_INTELFEATURES	= 0x80000001,
	};

public:
	enum FeatSource {
		BASIC,
		INTEL
	};

	enum Feature {
		// basic edx
		FEAT_FPU		= 1ULL << 0,
		FEAT_DBGEXT		= 1ULL << 2,
		FEAT_PSE		= 1ULL << 3,
		FEAT_TSC		= 1ULL << 4,
		FEAT_MSR		= 1ULL << 5,
		FEAT_PAE		= 1ULL << 6,
		FEAT_MCE		= 1ULL << 7,
		FEAT_CX8		= 1ULL << 8,
		FEAT_APIC		= 1ULL << 9,
		FEAT_SEP		= 1ULL << 11,
		FEAT_MTTR		= 1ULL << 12,
		FEAT_PGE		= 1ULL << 13,
		FEAT_MMX		= 1ULL << 23,
		FEAT_FXSR		= 1ULL << 24,
		FEAT_SSE		= 1ULL << 25,
		FEAT_SSE2		= 1ULL << 26,
		FEAT_HTT		= 1ULL << 28,
		FEAT_TM			= 1ULL << 29,

		// basic ecx
		FEAT_SSE3		= 1ULL << (32 + 0),
		FEAT_MONMWAIT	= 1ULL << (32 + 3),
		FEAT_VMX		= 1ULL << (32 + 5),
		FEAT_SSSE3		= 1ULL << (32 + 9),
		FEAT_FMA		= 1ULL << (32 + 12),
		FEAT_PCID		= 1ULL << (32 + 17),
		FEAT_SSE41		= 1ULL << (32 + 19),
		FEAT_SSE42		= 1ULL << (32 + 20),
		FEAT_POPCNT		= 1ULL << (32 + 23),
		FEAT_AES		= 1ULL << (32 + 25),
		FEAT_AVX		= 1ULL << (32 + 28),

		// intel edx
		FEAT_NX			= 1ULL << 20,
	};

	enum {
		CR0_PAGING_ENABLED			= 1 << 31,
		/* Determines whether the CPU can write to pages marked read-only */
		CR0_WRITE_PROTECT			= 1 << 16,
		/* Enables the native (internal) mechanism for reporting x87 FPU errors when set;
		 * enables the PC-style x87 FPU error reporting mechanism when clear. */
		CR0_NUMERIC_ERROR			= 1 << 5,
		/* Allows the saving of the x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 context on a task switch
		 * to be delayed until an x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instruction is actually executed
		 * by the new task. The processor sets this flag on every task switch and tests it when executing
		 * x87 FPU/MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
		CR0_TASK_SWITCHED			= 1 << 3,
		/* Indicates that the processor does not have an internal or external x87 FPU when set; indicates
		 * an x87 FPU is present when clear. This flag also affects the execution of
		 * MMX/SSE/SSE2/SSE3/SSSE3/SSE4 instructions. */
		CR0_EMULATE					= 1 << 2,
		/* Controls the interaction of the WAIT (or FWAIT) instruction with the TS flag (bit 3 of CR0).
		 * If the MP flag is set, a WAIT instruction generates a device-not-available exception (#NM) if
		 * the TS flag is also set. If the MP flag is clear, the WAIT instruction ignores the
		 * setting of the TS flag. */
		CR0_MONITOR_COPROC			= 1 << 1
	};

	enum {
		/* page size extension (enables 4MiB pages) */
		CR4_PSE			= 1 << 4,
		/* physical address extension */
		CR4_PAE			= 1 << 5,
		/* page global enable */
		CR4_PGE			= 1 << 7,
		/* for SSE/SSE2/SSE3/SSSE3/SSE4 */
		CR4_OSFXSR		= 1 << 9,
		/* for SIMD floating-point exception (#XM) */
		CR4_OSXMMEXCPT	= 1 << 10,
	};

	enum {
		EFLAG_IF					= 0x200,
	};

	enum {
		EFER_SCE					= 1 << 0,		// syscall enable
		EFER_LME					= 1 << 8,		// IA-32e mode enable
		EFER_LMA					= 1 << 10,		// IA-32e mode active
		EFER_NXE					= 1 << 11,		// Execute-disable bit enable
	};

	enum {
		MSR_IA32_SYSENTER_CS		= 0x174,
		MSR_IA32_SYSENTER_ESP		= 0x175,
		MSR_IA32_SYSENTER_EIP		= 0x176,
        MSR_EFER					= 0xc0000080,
        MSR_IA32_STAR              	= 0xc0000081,
        MSR_IA32_LSTAR             	= 0xc0000082,
        MSR_IA32_FMASK             	= 0xc0000084,
	};

	/**
	 * "Detects" the CPU
	 */
	static void detect();

	/**
	 * @param src the feature-source
	 * @param feat the feature
	 * @return whether the feature is supported
	 */
	static bool hasFeature(FeatSource src,uint64_t feat);

	/**
	 * Executes the 'pause' instruction
	 */
	static void pause() {
		asm volatile ("pause");
	}

	/**
	 * @return the bus speed in Hz
	 */
	static uint64_t getBusSpeed() {
		return busHz;
	}

	/**
	 * @return the value of a control-register
	 */
	static ulong getCR0() {
		ulong res;
		asm volatile ("mov %%cr0, %0" : "=r"(res));
		return res;
	}
	static ulong getCR2() {
		ulong res;
		asm volatile ("mov %%cr2, %0" : "=r"(res));
		return res;
	}
	static ulong getCR3() {
		ulong res;
		asm volatile ("mov %%cr3, %0" : "=r"(res));
		return res;
	}
	static ulong getCR4() {
		ulong res;
		asm volatile ("mov %%cr4, %0" : "=r"(res));
		return res;
	}

	/**
	 * Sets a control register
	 *
	 * @param val the new value
	 */
	static void setCR0(ulong val) {
		asm volatile ("mov %0, %%cr0" : : "r"(val));
	}
	static void setCR3(ulong val) {
		asm volatile ("mov %0, %%cr3" : : "r"(val));
	}
	static void setCR4(ulong val) {
		asm volatile ("mov %0, %%cr4" : : "r"(val));
	}

	/**
	 * @return the value of a debug-register
	 */
	static ulong getDR0() {
		ulong res;
		asm volatile ("mov %%dr0, %0" : "=r"(res));
		return res;
	}
	static ulong getDR1() {
		ulong res;
		asm volatile ("mov %%dr1, %0" : "=r"(res));
		return res;
	}
	static ulong getDR2() {
		ulong res;
		asm volatile ("mov %%dr2, %0" : "=r"(res));
		return res;
	}
	static ulong getDR3() {
		ulong res;
		asm volatile ("mov %%dr3, %0" : "=r"(res));
		return res;
	}
	static ulong getDR6() {
		ulong res;
		asm volatile ("mov %%dr6, %0" : "=r"(res));
		return res;
	}
	static ulong getDR7() {
		ulong res;
		asm volatile ("mov %%dr7, %0" : "=r"(res));
		return res;
	}

	/**
	 * Sets a debug register
	 *
	 * @param val the new value
	 */
	static void setDR0(ulong val) {
		asm volatile ("mov %0, %%dr0" : : "r"(val));
	}
	static void setDR1(ulong val) {
		asm volatile ("mov %0, %%dr1" : : "r"(val));
	}
	static void setDR2(ulong val) {
		asm volatile ("mov %0, %%dr2" : : "r"(val));
	}
	static void setDR3(ulong val) {
		asm volatile ("mov %0, %%dr3" : : "r"(val));
	}
	static void setDR7(ulong val) {
		asm volatile ("mov %0, %%dr7" : : "r"(val));
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

	/**
	 * Executes the cpuid instruction and stores the result to the given pointers.
	 */
	static void cpuid(unsigned code,uint32_t *eax,uint32_t *ebx,uint32_t *ecx,uint32_t *edx) {
		asm volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(code));
	}

	/**
	 * Executes the cpuid instruction and converts the result to a string
	 */
	static void cpuid(unsigned code,char *str) {
		uint32_t *buf = reinterpret_cast<uint32_t*>(str);
		uint32_t eax,ebx,ecx,edx;
		cpuid(code,&eax,&ebx,&ecx,&edx);
		buf[0] = ebx;
		buf[1] = edx;
		buf[2] = ecx;
	}

	/**
	 * @return the current stack-pointer
	 */
	A_ALWAYS_INLINE static ulong getSP() {
		ulong sp;
		GET_REG(sp,sp);
		return sp;
	}

private:
	static uint64_t cpuHz;
	static uint64_t busHz;
};

inline uint64_t CPUBase::rdtsc() {
	uint32_t u, l;
	asm volatile ("rdtsc" : "=a" (l), "=d" (u));
	return (uint64_t)u << 32 | l;
}

inline void CPUBase::halt() {
	asm volatile ("cli; hlt");
}

inline uint64_t CPUBase::getSpeed() {
	return CPU::cpuHz;
}
