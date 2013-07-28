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
#include <sys/task/thread.h>

class GDT {
	GDT() = delete;

public:
	/* the size of the io-map (in bits) */
	static const size_t IO_MAP_SIZE	= 0xFFFF;

private:
	/* the GDT table */
	struct Table {
		uint16_t size;		/* the size of the table -1 (size=0 is not allowed) */
		uint32_t offset;
	} A_PACKED;

	/* a GDT descriptor */
	struct Desc {
		/* size[0..15] */
		uint16_t sizeLow;

		/* address[0..15] */
		uint16_t addrLow;

		/* address[16..23] */
		uint8_t addrMiddle;

		/*
		 *     7      6   5     4     3    2      1        0
		 * |-------|----|----|------|----|---|---------|--------|
		 * |present|privilege|unused|exec|dir|readWrite|accessed|
		 * ------------------------------------------------------
		 *
		 * present:		This must be 1 for all valid selectors.
		 * privilege:	Contains the ring level, 0 = highest (kernel), 3 = lowest (user applications).
		 * unused:		always 1
		 * exec:		If 1 code in this segment can be executed
		 * dir:			Direction bit for data selectors: Tells the direction. 0 the segment grows up.
		 * 				1 the segment grows down, ie. the offset has to be greater than the base.
		 * 				Conforming bit for code selectors: Privilege-related.
		 * readWrite:	Readable bit for code selectors: Whether read access for this segment
		 * 				is allowed. Write access is never allowed for code segments.
		 * 				Writable bit for data selectors: Whether write access for this segment
		 * 				is allowed. Read access is always allowed for data segments.
		 * accessed:	Just set to 0. The CPU sets this to 1 when the segment is accessed.
		 */
		uint8_t access;

		/*
		 *       7       6   5   4  3  2  1  0
		 * |-----------|----|--|---|--|--|--|--|
		 * |granularity|size|unused|  sizeHigh |
		 * -------------------------------------
		 *
		 * granularity:	If 0 the limit is in 1 B blocks (byte granularity), if 1 the limit is in
		 * 				4 KiB blocks (page granularity).
		 * size:		If 0 the selector defines 16 bit protected mode. If 1 it defines 32 bit
		 * 				protected mode. You can have both 16 bit and 32 bit selectors at once.
		 * unused:		always 0
		 * sizeHigh:	size[16..19]
		 */
		uint8_t sizeHigh;

		/* address[24..31] */
		uint8_t addrHigh;
	} A_PACKED;

	/* the Task State Segment */
	struct TSS {
		/*
		 * Contains the segment selector for the TSS of the
		 * previous task (updated on a task switch that was initiated by a call, interrupt, or
		 * exception). This field (which is sometimes called the back link field) permits a
		 * task switch back to the previous task by using the IRET instruction.
		 */
		uint16_t prevTaskLink;
		uint16_t : 16; /* reserved */
		/* esp for PL 0 */
		uint32_t esp0;
		/* stack-segment for PL 0 */
		uint16_t ss0;
		uint16_t : 16; /* reserved */
		/* esp for PL 1 */
		uint32_t esp1;
		/* stack-segment for PL 1 */
		uint16_t ss1;
		uint16_t : 16; /* reserved */
		/* esp for PL 2 */
		uint32_t esp2;
		/* stack-segment for PL 2 */
		uint16_t ss2;
		uint16_t : 16; /* reserved */
		/* page-directory-base-register */
		uint32_t cr3;
		/* State of the EIP register prior to the task switch. */
		uint32_t eip;
		/* State of the EFAGS register prior to the task switch. */
		uint32_t eflags;
		/* general purpose registers */
		uint32_t eax;
		uint32_t ecx;
		uint32_t edx;
		uint32_t ebx;
		uint32_t esp;
		uint32_t ebp;
		uint32_t esi;
		uint32_t edi;
		/* segment registers */
		uint16_t es;
		uint16_t : 16; /* reserved */
		uint16_t cs;
		uint16_t : 16; /* reserved */
		uint16_t ss;
		uint16_t : 16; /* reserved */
		uint16_t ds;
		uint16_t : 16; /* reserved */
		uint16_t fs;
		uint16_t : 16; /* reserved */
		uint16_t gs;
		uint16_t : 16; /* reserved */
		/* Contains the segment selector for the task's LDT. */
		uint16_t ldtSegmentSelector;
		uint16_t : 16; /* reserved */
		/* When set, the T flag causes the processor to raise a debug exception when a task
		 * switch to this task occurs */
		uint16_t debugTrap	: 1,
						: 15; /* reserved */
		/* Contains a 16-bit offset from the base of the TSS to the I/O permission bit map
		 * and interrupt redirection bitmap. When present, these maps are stored in the
		 * TSS at higher addresses. The I/O map base address points to the beginning of the
		 * I/O permission bit map and the end of the interrupt redirection bit map. */
		uint16_t ioMapOffset;
		uint8_t ioMap[IO_MAP_SIZE / 8];
		uint8_t ioMapEnd;
	} A_PACKED;

	/* we need 6 entries: null-entry, code for kernel, data for kernel, user-code, user-data, tls
	 * and one entry for our TSS */
	static const size_t GDT_ENTRY_COUNT = 8;

public:
	/* the segment-indices in the gdt */
	enum {
		SEGSEL_GDTI_KCODE				= 1 << 3,
		SEGSEL_GDTI_KDATA				= 2 << 3,
		SEGSEL_GDTI_UCODE				= 3 << 3,
		SEGSEL_GDTI_UDATA				= 4 << 3,
		SEGSEL_GDTI_UTLS				= 5 << 3
	};

	/* the table-indicators */
	enum {
		SEGSEL_TI_GDT					= 0x00,
		SEGSEL_TI_LDT					= 0x04
	};

	/* the requested privilege levels */
	enum {
		SEGSEL_RPL_USER					= 0x03,
		SEGSEL_RPL_KERNEL				= 0x00
	};

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
	 * @param old the old thread (may be NULL)
	 * @param n the thread to run
	 * @return the cpu-id for the new thread
	 */
	static cpuid_t prepareRun(Thread *old,Thread *n);

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
	static void flush(Table *gdt) asm("gdt_flush");
	static void get(Table *gdt) {
		asm volatile ("sgdt (%0)\n" : : "r"(gdt));
	}
	static void loadTSS(size_t gdtOffset) {
		asm volatile ("ltr %0" : : "m"(gdtOffset));
	}
	static void setDesc(Desc *gdt,size_t index,uintptr_t address,size_t size,uint8_t access,
	                    uint8_t ringLevel);
	static void setTSSDesc(Desc *gdt,size_t index,uintptr_t address,size_t size);

	/* the GDTs */
	static Desc bspgdt[GDT_ENTRY_COUNT];
	static Table *allgdts;
	static size_t gdtCount;

	/* our TSS's (should not contain a page-boundary) */
	static TSS bsptss;
	static TSS **alltss;
	static size_t tssCount;

	/* I/O maps for all TSSs; just to remember the last set I/O map */
	static const uint8_t **ioMaps;
	static size_t cpuCount;
};
