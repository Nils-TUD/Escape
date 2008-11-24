/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVGDT_H_
#define PRIVGDT_H_

#include "../pub/common.h"
#include "../pub/gdt.h"

/* the GDT table */
typedef struct {
	u16 size;		/* the size of the table -1 (size=0 is not allowed) */
	u32 offset;
} __attribute__((packed)) sGDTTable;

/* a GDT descriptor */
typedef struct {
	/* size[0..15] */
	u16 sizeLow;

	/* address[0..15] */
	u16 addrLow;

	/* address[16..23] */
	u8 addrMiddle;

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
	 * 				(TODO Please add more information)
	 * readWrite:	Readable bit for code selectors: Whether read access for this segment
	 * 				is allowed. Write access is never allowed for code segments.
	 * 				Writable bit for data selectors: Whether write access for this segment
	 * 				is allowed. Read access is always allowed for data segments.
	 * accessed:	Just set to 0. The CPU sets this to 1 when the segment is accessed.
	 */
	u8 access;

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
	u8 sizeHigh;

	/* address[24..31] */
	u8 addrHigh;
} __attribute__((packed)) sGDTDesc;

/* the Task State Segment */
typedef struct {
	/*
	 * Contains the segment selector for the TSS of the
	 * previous task (updated on a task switch that was initiated by a call, interrupt, or
	 * exception). This field (which is sometimes called the back link field) permits a
	 * task switch back to the previous task by using the IRET instruction.
	 */
	u16 prevTaskLink;
	u16 : 16; /* reserved */
	/* esp for PL 0 */
	u32 esp0;
	/* stack-segment for PL 0 */
	u16 ss0;
	u16 : 16; /* reserved */
	/* esp for PL 1 */
	u32 esp1;
	/* stack-segment for PL 1 */
	u16 ss1;
	u16 : 16; /* reserved */
	/* esp for PL 2 */
	u32 esp2;
	/* stack-segment for PL 2 */
	u16 ss2;
	u16 : 16; /* reserved */
	/* page-directory-base-register */
	u32 cr3;
	/* State of the EIP register prior to the task switch. */
	u32 eip;
	/* State of the EFAGS register prior to the task switch. */
	u32 eflags;
	/* general purpose registers */
	u32 eax;
	u32 ecx;
	u32 edx;
	u32 ebx;
	u32 esp;
	u32 ebp;
	u32 esi;
	u32 edi;
	/* segment registers */
	u16 es;
	u16 : 16; /* reserved */
	u16 cs;
	u16 : 16; /* reserved */
	u16 ss;
	u16 : 16; /* reserved */
	u16 ds;
	u16 : 16; /* reserved */
	u16 fs;
	u16 : 16; /* reserved */
	u16 gs;
	u16 : 16; /* reserved */
	/* Contains the segment selector for the task's LDT. */
	u16 ldtSegmentSelector;
	u16 : 16; /* reserved */
	/* When set, the T flag causes the processor to raise a debug exception when a task
	 * switch to this task occurs */
	u16 debugTrap	: 1,
					: 15; /* reserved */
	/* Contains a 16-bit offset from the base of the TSS to the I/O permission bit map
	 * and interrupt redirection bitmap. When present, these maps are stored in the
	 * TSS at higher addresses. The I/O map base address points to the beginning of the
	 * I/O permission bit map and the end of the interrupt redirection bit map. */
	u16 ioMapBaseAddr;
} __attribute__((packed)) sTSS;

/* we need 6 entries: null-entry, code for kernel, data for kernel, user-code, user-data
 * and one entry for our TSS */
#define GDT_ENTRY_COUNT 6

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
extern void tss_load(u16 gdtOffset);

/**
 * Sets the descriptor with given index to the given attributes
 *
 * @param index the index of the descriptor
 * @param address the address of the segment
 * @param size the size of the segment (in pages)
 * @param access the access-byte
 * @param ringLevel the ring-level for the segment (0 = kernel, 3 = user)
 */
static void gdt_set_desc(u16 index,u32 address,u32 size,u8 access,u8 ringLevel);

/**
 * Sets the TSS descriptor with the given index and given attributes
 *
 * @param index the index of the descriptor
 * @param address the address of the segment
 * @param size the size of the segment (in bytes)
 */
static void gdt_set_tss_desc(u16 index,u32 address,u32 size);

#endif /* PRIVGDT_H_ */
