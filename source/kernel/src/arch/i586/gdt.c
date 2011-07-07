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

#include <sys/common.h>
#include <sys/arch/i586/gdt.h>
#include <sys/mem/pmem.h>
#include <sys/mem/paging.h>
#include <sys/video.h>
#include <string.h>
/* for offsetof() */
#include <stddef.h>

/* we need 6 entries: null-entry, code for kernel, data for kernel, user-code, user-data, tls
 * and one entry for our TSS */
#define GDT_ENTRY_COUNT 7

/* == for the access-field == */

/* the GDT entry types */
#define GDT_TYPE_DATA			(0 | GDT_NOSYS)
#define GDT_TYPE_CODE			(1 << 3 | GDT_NOSYS)
#define GDT_TYPE_32BIT_TSS		(1 << 3 | 1 << 0)	/* requires GDT_NOSYS_SEG = 0 */

/* flag to switch between system- and code/data-segment */
#define GDT_NOSYS				(1 << 4)
#define GDT_PRESENT				(1 << 7)
#define GDT_DPL					(1 << 5 | 1 << 6)

#define GDT_DATA_WRITE			(1 << 1)
#define GDT_CODE_READ			(1 << 1)
#define GDT_CODE_CONFORMING		(1 << 2)

#define GDT_32BIT_PMODE			(1 << 6)
#define GDT_PAGE_GRANULARITY	(1 << 7)

/* privilege level */
#define GDT_DPL_KERNEL			0
#define GDT_DPL_USER			3

/* the offset of the io-bitmap */
#define IO_MAP_OFFSET			offsetof(sTSS,ioMap)
/* an invalid offset for the io-bitmap => not loaded yet */
#define IO_MAP_OFFSET_INVALID	sizeof(sTSS) + 16

/* the GDT table */
typedef struct {
	uint16_t size;		/* the size of the table -1 (size=0 is not allowed) */
	uint32_t offset;
} A_PACKED sGDTTable;

/* a GDT descriptor */
typedef struct {
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
} A_PACKED sGDTDesc;

/* the Task State Segment */
typedef struct {
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
} A_PACKED sTSS;

/**
 * Assembler routine to flush the GDT
 *
 * @param gdt the pointer to the GDT-pointer
 */
extern void gdt_flush(sGDTTable *gdt);

/**
 * Loads the TSS at the given offset in the GDT
 *
 * @param gdtOffset the offset
 */
extern void tss_load(size_t gdtOffset);

/**
 * Sets the descriptor with given index to the given attributes
 *
 * @param index the index of the descriptor
 * @param address the address of the segment
 * @param size the size of the segment (in pages)
 * @param access the access-byte
 * @param ringLevel the ring-level for the segment (0 = kernel, 3 = user)
 */
static void gdt_set_desc(size_t index,uintptr_t address,size_t size,uint8_t access,
		uint8_t ringLevel);

/**
 * Sets the TSS descriptor with the given index and given attributes
 *
 * @param index the index of the descriptor
 * @param address the address of the segment
 * @param size the size of the segment (in bytes)
 */
static void gdt_set_tss_desc(size_t index,uintptr_t address,size_t size);

/* the GDT */
static sGDTDesc gdt[GDT_ENTRY_COUNT];

/* our TSS (should not contain a page-boundary) */
static sTSS tss A_ALIGNED(PAGE_SIZE);

void gdt_init(void) {
	sGDTTable gdtTable;
	gdtTable.offset = (uintptr_t)gdt;
	gdtTable.size = GDT_ENTRY_COUNT * sizeof(sGDTDesc) - 1;

	/* clear gdt */
	memclear(gdt,GDT_ENTRY_COUNT * sizeof(sGDTDesc));

	/* kernel code */
	gdt_set_desc(1,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_KERNEL);
	/* kernel data */
	gdt_set_desc(2,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_KERNEL);

	/* user code */
	gdt_set_desc(3,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_CODE | GDT_PRESENT | GDT_CODE_READ,GDT_DPL_USER);
	/* user data */
	gdt_set_desc(4,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);
	/* tls */
	gdt_set_desc(5,0,0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);

	/* tss (leave a bit space for the vm86-segment-registers that will be present at the stack-top
	 * in vm86-mode. This way we can have the same interrupt-stack for all processes) */
	tss.esp0 = KERNEL_STACK + PAGE_SIZE - (1 + 4) * sizeof(int);
	tss.ss0 = 0x10;
	/* init io-map */
	tss.ioMapOffset = IO_MAP_OFFSET_INVALID;
	tss.ioMapEnd = 0xFF;
	gdt_set_tss_desc(6,(uintptr_t)&tss,sizeof(sTSS) - 1);

	/* now load the GDT */
	gdt_flush(&gdtTable);

	/* load tss */
	tss_load(6 * sizeof(sGDTDesc));

	/* We needed the area 0x0 .. 0x00400000 because in the first phase the GDT was setup so that
	 * the stuff at 0xC0000000 has been mapped to 0x00000000. Therefore, after enabling paging
	 * (the GDT is still the same) we have to map 0x00000000 to our kernel-stuff so that we can
	 * still access the kernel (because segmentation translates 0xC0000000 to 0x00000000 before
	 * it passes it to the MMU).
	 * Now our GDT is setup in the "right" way, so that 0xC0000000 will arrive at the MMU.
	 * Therefore we can unmap the 0x0 area. */
	paging_gdtFinished();
}

void gdt_setTLS(uintptr_t tlsAddr,size_t tlsSize) {
	/* the thread-control-block is at the end of the tls-region; %gs:0x0 should reference
	 * the thread-control-block; use 0xFFFFFFFF as limit because we want to be able to use
	 * %gs:0xFFFFFFF8 etc. */
	gdt_set_desc(5,(tlsAddr + tlsSize - sizeof(void*)),0xFFFFFFFF >> PAGE_SIZE_SHIFT,
			GDT_TYPE_DATA | GDT_PRESENT | GDT_DATA_WRITE,GDT_DPL_USER);
}

void tss_setStackPtr(bool isVM86) {
	/* VM86-tasks should start at the beginning because the segment-registers are saved on the
	 * stack first (not in protected mode) */
	if(isVM86)
		tss.esp0 = KERNEL_STACK + PAGE_SIZE - sizeof(int);
	else
		tss.esp0 = KERNEL_STACK + PAGE_SIZE - (1 + 4) * sizeof(int);
}

bool tss_ioMapPresent(void) {
	return tss.ioMapOffset != IO_MAP_OFFSET_INVALID;
}

void tss_setIOMap(const uint8_t *ioMap) {
	tss.ioMapOffset = IO_MAP_OFFSET;
	memcpy(tss.ioMap,ioMap,IO_MAP_SIZE / 8);
}

void tss_removeIOMap(void) {
	tss.ioMapOffset = IO_MAP_OFFSET_INVALID;
}

static void gdt_set_tss_desc(size_t index,uintptr_t address,size_t size) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_32BIT_PMODE;
	gdt[index].access = GDT_PRESENT | GDT_TYPE_32BIT_TSS | (GDT_DPL_KERNEL << 5);
}

static void gdt_set_desc(size_t index,uintptr_t address,size_t size,uint8_t access,
		uint8_t ringLevel) {
	gdt[index].addrLow = address & 0xFFFF;
	gdt[index].addrMiddle = (address >> 16) & 0xFF;
	gdt[index].addrHigh = (address >> 24) & 0xFF;
	gdt[index].sizeLow = size & 0xFFFF;
	gdt[index].sizeHigh = ((size >> 16) & 0xF) | GDT_PAGE_GRANULARITY | GDT_32BIT_PMODE;
	gdt[index].access = access | ((ringLevel & 3) << 5);
}

void gdt_print(void) {
	size_t i;
	vid_printf("GDT:\n");
	for(i = 0;i < GDT_ENTRY_COUNT; i++) {
		vid_printf("\t%d: address=%02x%02x:%04x, size=%02x%04x, access=%02x\n",
				i,gdt[i].addrHigh,gdt[i].addrMiddle,gdt[i].addrLow,
				gdt[i].sizeHigh,gdt[i].sizeLow,gdt[i].access);
	}
}
