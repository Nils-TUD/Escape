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

#include <arch/x86/desc.h>
#include <arch/x86/tss.h>
#include <sys/arch.h>
#include <task/thread.h>
#include <common.h>

class GDT {
	GDT() = delete;

	struct PerCPU {
		DescTable gdt;
		TSS *tss;
		const uint8_t *ioMap;
		uint32_t lastMSR;
	};

	/* we need 6 entries: null-entry, code for kernel, data for kernel, user-code, user-data, tls
	 * and one entry for our TSS (2 Descs for x86_64) */
	static const size_t GDT_ENTRY_COUNT = 10;

public:
	/**
	 * Inits the GDT
	 */
	static void init();

	/**
	 * Finishes the initialization for the bootstrap processor
	 */
	static void initBSP();

	/**
	 * Finishes the initialization for an application processor
	 */
	static void initAP();

	/**
	 * @return the current cpu-id
	 */
	static cpuid_t getCPUId();

	/**
	 * Sets the running thread for the given CPU
	 *
	 * @param id the cpu-id
	 * @param t the thread
	 */
	static void setRunning(cpuid_t id,Thread *t);

	/**
	 * Prepares the run of the given thread, i.e. sets the stack-pointer for interrupts, removes
	 * the I/O map and sets the TLS-register.
	 *
	 * @param id the CPU id
	 * @param newProc whether it is a new process
	 * @param n the thread to run
	 */
	static void prepareRun(cpuid_t id,bool newProc,Thread *n);

	/**
	 * Checks whether the io-map is set
	 *
	 * @return true if so
	 */
	static bool ioMapPresent();

	/**
	 * Sets the given io-map. That means it copies IO_MAP_SIZE / 8 bytes from the given address
	 * into the TSS
	 *
	 * @param ioMap the io-map to set
	 * @param forceCpy whether to force a copy of the given map (necessary if it has changed)
	 */
	static void setIOMap(const uint8_t *ioMap,bool forceCpy);

	/**
	 * Prints the GDT
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	static ulong selector(int seg) {
		return (seg << 3) | 0x3;
	}
	static void flush(volatile DescTable *gdt) {
		asm volatile ("lgdt	(%0)" : : "r"(gdt));
	}
	static void get(DescTable *gdt) {
		asm volatile ("sgdt (%0)\n" : : "r"(gdt) : "memory");
	}
	static void loadTSS(size_t gdtOffset) {
		asm volatile ("ltr %0" : : "m"(gdtOffset));
	}
	static void setTSS(Desc *gdt,TSS *tss,uintptr_t kstack);
	static void setDesc(Desc *d,uintptr_t address,size_t limit,uint8_t granu,uint8_t type,uint8_t dpl);
	static void setDesc64(Desc *d,uintptr_t address,size_t limit,uint8_t granu,uint8_t type,uint8_t dpl);
	static void setupSyscalls(TSS *tss);

	/* for the BSP (we don't have Cache::alloc yet) */
	static Desc bspgdt[GDT_ENTRY_COUNT];
	static TSS bspTSS;
	static PerCPU bsp;

	static PerCPU *all;
	static size_t cpuCount;
};
