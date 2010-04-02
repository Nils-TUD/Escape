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

#ifndef PAGING_H_
#define PAGING_H_

#include <common.h>
#include <mem/paging.h>
#include <mem/pmem.h>
#include <task/proc.h>
#include <task/thread.h>
#include <printf.h>

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |             user-code             |     |
 *             |                                   |     |
 *             +-----------------------------------+     |
 *             |                                   |
 *      |      |             user-data             |     u
 *      v      |                                   |     s
 *             +-----------------------------------+     e
 *             |                ...                |     r
 *     ---     +-----------------------------------+     a
 *      ^      |                                   |     r
 *      |      |        user-stack thread n        |     e
 *             |                                   |     a
 *             +-----------------------------------+
 *             |                ...                |     |
 *     ---     +-----------------------------------+     |
 *      ^      |                                   |     |
 *      |      |        user-stack thread 0        |     |
 *             |                                   |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |                ...                |     |
 * 0xC0100000: +-----------------------------------+     |
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |     k
 *      v      +-----------------------------------+     e
 *             |                ...                |     r
 * 0xC0800000: +-----------------------------------+     n
 *             |                                   |     e
 *      |      |            kernel-heap            |     l
 *      v      |                                   |     a
 * 0xC1C00000: +-----------------------------------+     r
 *             |           temp map area           |     e
 * 0xC2800000: +-----------------------------------+     a
 *             |                ...                |
 * 0xFF7FE000: +-----------------------------------+     |      -----
 *             |            kernel-stack           |     |        |
 * 0xFF7FF000: +-----------------------------------+     |        |
 *             |         temp kernel-stack         |     |
 * 0xFF800000: +-----------------------------------+     |     not shared page-tables (3)
 *             |     temp mapped page-tables       |     |
 * 0xFFC00000: +-----------------------------------+     |        |
 *             |        mapped page-tables         |     |        |
 * 0xFFFFFFFF: +-----------------------------------+   -----    -----
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA_V_ADDR	((u32)0xC0000000)
/* the virtual address of the kernel itself */
#define KERNEL_V_ADDR		(KERNEL_AREA_V_ADDR + KERNEL_P_ADDR)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT		(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START	(0xFFFFFFFF - (PT_ENTRY_COUNT * PAGE_SIZE) + 1)
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START	(MAPPED_PTS_START - (PT_ENTRY_COUNT * PAGE_SIZE))
/* the start of the kernel-heap */
#define KERNEL_HEAP_START	(KERNEL_AREA_V_ADDR + (PT_ENTRY_COUNT * PAGE_SIZE) * 2)
/* the size of the kernel-heap (4 MiB) */
#define KERNEL_HEAP_SIZE	(PT_ENTRY_COUNT * PAGE_SIZE /* * 4 */)

/* page-directories in virtual memory */
#define PAGE_DIR_AREA		(MAPPED_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA	(TMPMAP_PTS_START + PAGE_SIZE * (PT_ENTRY_COUNT - 1))
/* our kernel-stack */
#define KERNEL_STACK		(TEMP_MAP_PAGE - PAGE_SIZE)
/* temporary page for various purposes */
#define TEMP_MAP_PAGE		(TMPMAP_PTS_START - PAGE_SIZE)

/* for mapping some pages of foreign processes */
#define TEMP_MAP_AREA		(KERNEL_HEAP_START + KERNEL_HEAP_SIZE)
#define TEMP_MAP_AREA_SIZE	(PT_ENTRY_COUNT * PAGE_SIZE * 4)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE		PAGE_SIZE

/* flags for paging_map() */
#define PG_WRITABLE			1
#define PG_SUPERVISOR		2
#define PG_PRESENT			4
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME	8
/* make it a global page */
#define PG_GLOBAL			16
/* tells paging_map() to keep the currently set frame-number */
#define PG_KEEPFRM			32

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr) ((u32)(addr) / PAGE_SIZE / PT_ENTRY_COUNT)

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr) (((u32)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT)

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount) (((u32)(pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

#define PAGEDIR(ptables)	((u32)(ptables) + PAGE_SIZE * (PT_ENTRY_COUNT - 1))

/* for printing the page-directory */
#define PD_PART_ALL		0
#define PD_PART_USER 	1
#define PD_PART_KERNEL	2
#define PD_PART_KHEAP	4
#define PD_PART_PTBLS	8
#define PD_PART_TEMPMAP	16

/* start-address of the text */
#define TEXT_BEGIN		0x1000

/* represents a page-directory-entry */
typedef struct {
	/* 1 if the page is present in memory */
	u32 present			: 1,
	/* whether the page is writable */
	writable			: 1,
	/* if enabled the page may be used by privilege level 0, 1 and 2 only. */
	notSuperVisor		: 1,
	/* >= 80486 only. if enabled everything will be written immediatly into memory */
	pageWriteThrough	: 1,
	/* >= 80486 only. if enabled the CPU will not put anything from the page in the cache */
	pageCacheDisable	: 1,
	/* enabled if the page-table has been accessed (has to be cleared by the OS!) */
	accessed			: 1,
	/* 1 ignored bit */
						: 1,
	/* whether the pages are 4 KiB (=0) or 4 MiB (=1) large */
	pageSize			: 1,
	/* 1 ignored bit */
						: 1,
	/* can be used by the OS */
	/* Indicates that this pagetable exists (but must not necessarily be present) */
	exists				: 1,
	/* unused */
						: 2,
	/* the physical address of the page-table */
	ptFrameNo			: 20;
} sPDEntry;

/* represents a page-table-entry */
typedef struct {
	/* 1 if the page is present in memory */
	u32 present			: 1,
	/* whether the page is writable */
	writable			: 1,
	/* if enabled the page may be used by privilege level 0, 1 and 2 only. */
	notSuperVisor		: 1,
	/* >= 80486 only. if enabled everything will be written immediatly into memory */
	pageWriteThrough	: 1,
	/* >= 80486 only. if enabled the CPU will not put anything from the page in the cache */
	pageCacheDisable	: 1,
	/* enabled if the page has been accessed (has to be cleared by the OS!) */
	accessed			: 1,
	/* enabled if the page can not be removed currently (has to be cleared by the OS!) */
	dirty				: 1,
	/* 1 ignored bit */
						: 1,
	/* The Global, or 'G' above, flag, if set, prevents the TLB from updating the address in
	 * it's cache if CR3 is reset. Note, that the page global enable bit in CR4 must be set
	 * to enable this feature. (>= pentium pro) */
	global				: 1,
	/* 3 Bits for the OS */
	/* Indicates that this page exists (but must not necessarily be present) */
	exists				: 1,
	/* unused */
						: 2,
	/* the physical address of the page */
	frameNumber			: 20;
} sPTEntry;

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Sets the current page-dir. Has to be called on every context-switch!
 *
 * @param pdir the pagedir
 */
void paging_setCur(tPageDir pdir);

/**
 * Reserves page-tables for the whole higher-half and inserts them into the page-directory.
 * This should be done ONCE at the beginning as soon as the physical memory management is set up
 */
void paging_mapKernelSpace(void);

/**
 * Note that this should just be used by proc_init()!
 *
 * @return the address of the page-directory of process 0
 */
sPDEntry *paging_getProc0PD(void);

/**
 * Assembler routine to flush the TLB
 */
extern void paging_flushTLB(void);

/**
 * Flushes the TLB-entry for the given address
 *
 * @param address the virtual address
 */
extern void paging_flushAddr(u32 address);

/**
 * Assembler routine to exchange the page-directory to the given one
 *
 * @param physAddr the physical address of the page-directory
 */
extern void paging_exchangePDir(u32 physAddr);

/**
 * Checks whether the given address-range is currently readable for the user
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserReadable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently readable
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeReadable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently writable for the user.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserWritable(u32 virt,u32 count);

/**
 * Checks whether the given address-range is currently writable.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeWritable(u32 virt,u32 count);

/**
 * Clones the kernel-space of the current page-dir into a new one.
 *
 * @param stackFrame will be set to the used kernel-stackframe
 * @return the created pagedir
 */
u32 paging_cloneKernelspace(u32 *stackFrame);

/**
 * Destroys the given page-directory (not the current!)
 *
 * @param pdir the page-dir
 * @return the number of free'd frames
 */
u32 paging_destroyPDir(tPageDir pdir);

/**
 * @param virt the virtual address
 * @return the frame-number of the given virtual address
 */
u32 paging_getFrameNo(u32 virt);

/**
 * Clones <count> pages at <virtSrc> to <virtDst> from <src> into <dst>. That means
 * the flags are copied. If <share> is true, the frames will be copied as well. Otherwise new
 * frames will be allocated if the page in <src> is present. Additionally, if <share> is false
 * all present pages will be marked as not-writable and share the frame in the cloned pagedir
 * so that they can be copied on write-access. Note that either <src> or <dst> has to be the
 * current page-dir!
 *
 * @param src the source-pagedir
 * @param dst the destination-pagedir
 * @param virtSrc the virtual source address
 * @param virtDst the virtual destination address
 * @param count the number of pages to copy
 * @param share wether to share the frames
 * @return the number of allocated frames (including page-tables)
 */
u32 paging_clonePages(tPageDir src,tPageDir dst,u32 virtSrc,u32 virtDst,u32 count,bool share);

/**
 * Maps <count> virtual addresses starting at <virt> to the given frames in the CURRENT page-directory.
 * Note that the frame-number will just be set (and thus, <frames> used) when the flag PG_PRESENT
 * is specified.
 *
 * @param virt the virt start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @return the number of allocated frames (including page-tables)
 */
u32 paging_map(u32 virt,u32 *frames,u32 count,u8 flags);

/**
 * Maps <count> virtual addresses starting at <virt> to the given frames in the given page-directory.
 * Note that the frame-number will just be set (and thus, <frames> used) when the flag PG_PRESENT
 * is specified.
 *
 * @param pdir the physical address of the page-directory
 * @param virt the virt start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @return the number of allocated frames (including page-tables)
 */
u32 paging_mapTo(tPageDir pdir,u32 virt,u32 *frames,u32 count,u8 flags);

/**
 * Removes <count> pages starting at <virt> from the page-tables in the CURRENT page-directory.
 * If you like the function free's the frames.
 * Note that the function will delete page-tables, too, if they are empty!
 *
 * @param virt the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames whether the frames should be free'd and not just unmapped
 * @return the number of free'd frames
 */
u32 paging_unmap(u32 virt,u32 count,bool freeFrames);

/**
 * Removes <count> pages starting at <virt> from the page-tables in the given page-directory.
 * If you like the function free's the frames.
 * Note that the function will delete page-tables, too, if they are empty!
 *
 * @param pdir the physical address of the page-directory
 * @param virt the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames whether the frames should be free'd and not just unmapped
 * @return the number of free'd frames
 */
u32 paging_unmapFrom(tPageDir pdir,u32 virt,u32 count,bool freeFrames);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);


#if DEBUGGING

/**
 * Counts the number of pages that are currently present in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of pages
 */
u32 paging_dbg_getPageCount(void);

/**
 * Prints the given parts of the current page-directory
 *
 * @param parts the parts to print
 */
void paging_dbg_printCur(u8 parts);

/**
 * Prints the given parts from the given page-directory
 *
 * @param pdir the page-directory
 * @param parts the parts to print
 */
void paging_dbg_printPDir(tPageDir pdir,u8 parts);

#endif

#endif /*PAGING_H_*/
