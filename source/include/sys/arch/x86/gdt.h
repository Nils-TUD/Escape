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
#include <sys/task/thread.h>
#include <sys/arch/x86/tss.h>
#include <esc/arch.h>

class GDT {
	GDT() = delete;

	/* the GDT table */
	struct Table {
		uint16_t size;		/* the size of the table -1 (size=0 is not allowed) */
		ulong offset;
	} A_PACKED;

	/* a GDT descriptor */
	struct Desc {
		/* limit[0..15] */
		uint16_t limitLow;

		/* address[0..15] */
		uint16_t addrLow;

		/* address[16..23] */
		uint8_t addrMiddle;

		/*
		 * present:		This must be 1 for all valid selectors.
		 * dpl:			Contains the ring level, 0 = highest (kernel), 3 = lowest (user applications).
		 * type:		segment type
		 */
		uint8_t type : 5,
				dpl : 2,
				present : 1;

		/*
		 * granularity:	If 0 the limit is in 1 B blocks (byte granularity), if 1 the limit is in
		 * 				4 KiB blocks (page granularity).
		 * size:		If 0 the selector defines 16 bit protected mode. If 1 it defines 32 bit
		 * 				protected mode. You can have both 16 bit and 32 bit selectors at once.
		 * bits:		1 = 64-bit
		 * unused:		can be used by system-software
		 * limitHigh:	limit[16..19]
		 */
		uint8_t limitHigh : 4,
				/* unused */ : 1,
				bits : 1,
				size : 1,
				granularity : 1;

		/* address[24..31] */
		uint8_t addrHigh;
	} A_PACKED;

	enum {
		SYS_TSS		= 0x09,
		DATA_RO		= 0x10,
		DATA_RW		= 0x12,
		CODE_X		= 0x18,
		CODE_XR		= 0x1A,
	};

	enum {
		GRANU_BYTES	= 0x0,
		GRANU_PAGES	= 0x1,
	};

	enum {
		SIZE_16		= 0x0,
		SIZE_32		= 0x1,
	};

	enum {
		DPL_KERNEL	= 0x0,
		DPL_USER	= 0x3,
	};

	struct PerCPU {
		Table gdt;
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
	static void flush(Table *gdt) {
		asm volatile ("lgdt	(%0)" : : "r"(gdt));
	}
	static void get(Table *gdt) {
		asm volatile ("sgdt (%0)\n" : : "r"(gdt));
	}
	static void loadTSS(size_t gdtOffset) {
		asm volatile ("ltr %0" : : "m"(gdtOffset));
	}
	static void setTSS(Desc *gdt,TSS *tss,uintptr_t kstack);
	static void setDesc(Desc *d,uintptr_t address,size_t limit,uint8_t granu,uint8_t type,uint8_t dpl);
	static void setupSyscalls(TSS *tss);

	/* for the BSP (we don't have Cache::alloc yet) */
	static Desc bspgdt[GDT_ENTRY_COUNT];
	static TSS bspTSS;
	static PerCPU bsp;

	static PerCPU *all;
	static size_t cpuCount;
};
