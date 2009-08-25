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

#include "common.h"
#include "mm.h"
#include "proc.h"
#include "thread.h"
#include "util.h"

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
 * 0xC0400000: +-----------------------------------+     n
 *             |                                   |     e
 *      |      |            kernel-heap            |     l
 *      v      |                                   |     a
 * 0xC1800000: +-----------------------------------+     r
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
#define KERNEL_HEAP_START	(KERNEL_AREA_V_ADDR + (PT_ENTRY_COUNT * PAGE_SIZE))
/* the size of the kernel-heap (16 MiB) */
#define KERNEL_HEAP_SIZE	(PT_ENTRY_COUNT * PAGE_SIZE/* * 4*/)

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
#define PG_COPYONWRITE		4
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME	8
#define PG_NOFREE			16
/* tells paging_map() that the pages are inherited from another process; assumes PG_ADDR_TO_FRAME! */
#define PG_INHERIT			32

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr) ((u32)(addr) / PAGE_SIZE / PT_ENTRY_COUNT)

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr) (((u32)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT)

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount) (((pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

/* for printing the page-directory */
#define PD_PART_ALL		0
#define PD_PART_USER 	1
#define PD_PART_KERNEL	2
#define PD_PART_KHEAP	4
#define PD_PART_PTBLS	8
#define PD_PART_TEMPMAP	16

/* represents a page-directory-entry */
typedef struct {
	/* 1 if the page is present in memory */
	u32 present			: 1,
	/* wether the page is writable */
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
	/* wether the pages are 4 KiB (=0) or 4 MiB (=1) large */
	pageSize			: 1,
	/* 1 ignored bit */
						: 1,
	/* can be used by the OS */
						: 3,
	/* the physical address of the page-table */
	ptFrameNo			: 20;
} sPDEntry;

/* represents a page-table-entry */
typedef struct {
	/* 1 if the page is present in memory */
	u32 present			: 1,
	/* wether the page is writable */
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
	/* Indicates wether this page is currently readonly, shared with another process and should
	 * be copied as soon as the user writes to it */
	copyOnWrite			: 1,
	/* Indicates that the frame should not be free'd when this page is removed */
	noFree				: 1,
						: 1,
	/* the physical address of the page */
	frameNumber			: 20;
} sPTEntry;

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Reserves page-tables for the whole higher-half and inserts them into the page-directory.
 * This should be done ONCE at the beginning as soon as the physical memory management is set up
 */
void paging_mapHigherHalf(void);

/**
 * Inits the COW-list. Is possible as soon as the kheap is initialized.
 */
void paging_initCOWList(void);

/**
 * Note that this should just be used by proc_init()!
 *
 * @return the address of the page-directory of process 0
 */
sPDEntry *paging_getProc0PD(void);

/**
 * Handles a page-fault for the given address
 *
 * @param address the address that caused the page-fault
 * @return true if the page-fault could be handled
 */
bool paging_handlePageFault(u32 address);

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
 * Checks wether the given virtual-address is currently mapped. This should not be used
 * for user-space addresses!
 *
 * @param virt the virt address
 * @return true if so
 */
bool paging_isMapped(u32 virt);

/**
 * Checks wether the given address-range is currently readable for the user
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserReadable(u32 virt,u32 count);

/**
 * Checks wether the given address-range is currently readable
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeReadable(u32 virt,u32 count);

/**
 * Checks wether the given address-range is currently writable for the user.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserWritable(u32 virt,u32 count);

/**
 * Checks wether the given address-range is currently writable.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virt the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeWritable(u32 virt,u32 count);

/**
 * Determines the frame-number for the given virtual-address. This should not be used
 * for user-space addresses!
 *
 * @param virt the virt address
 * @return the frame-number to which it is currently mapped or 0 if not mapped
 */
u32 paging_getFrameOf(u32 virt);

/**
 * Determines how many new frames we need for calling paging_map(<virt>,...,<count>,...).
 *
 * @param virt the virtual start-address
 * @param count the number of pages to map
 * @return the number of new frames we would need
 */
u32 paging_countFramesForMap(u32 virt,u32 count);

/**
 * Maps the pages specified by <addr> and <size> from the process <p> into the current address-
 * space and returns the address of it.
 *
 * @param p the process
 * @param addr the start of the range you want to access
 * @param size the size of the range you want to access
 * @return the address you can use
 */
void *paging_mapAreaOf(sProc *p,u32 addr,u32 size);

/**
 * Unmaps the area previously mapped by paging_mapAreaOf().
 *
 * @param addr the start of the range you want to access
 * @param size the size of the range you want to access
 */
void paging_unmapArea(u32 addr,u32 size);

/**
 * Copies <count> pages from <vaddr> of the given process to the current one
 *
 * @param p the process
 * @param srcAddr the virtual address to copy from
 * @param dstAddr the virtual address to copy to
 * @param count the number of pages
 * @param flags flags for the pages (PG_*)
 */
void paging_getPagesOf(sProc *p,u32 srcAddr,u32 dstAddr,u32 count,u8 flags);

/**
 * Removes <count> pages at <addr> from the address-space of the given process
 *
 * @param p the process
 * @param addr the start-address
 * @param count the number of pages
 */
void paging_remPagesOf(sProc *p,u32 addr,u32 count);

/**
 * Maps <count> virtual addresses starting at <virt> to the given frames (in the CURRENT
 * page-dir!). You can decide (via <force>) wether the mapping should be done in every
 * case or just if the page is not already mapped.
 *
 * @panic if there is not enough memory to get a frame for a page-table
 *
 * @param virt the virt start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @param force wether the mapping should be overwritten
 */
void paging_map(u32 virt,u32 *frames,u32 count,u8 flags,bool force);

/**
 * Removes <count> pages starting at <virt> from the page-tables (in the CURRENT page-dir!).
 * If you like the function free's the frames.
 * Note that the function will NOT delete page-tables!
 *
 * @param virt the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames wether the frames should be free'd and not just unmapped
 * @param remCOW wether the frames should be removed from the COW-list
 */
void paging_unmap(u32 virt,u32 count,bool freeFrames,bool remCOW);

/**
 * Unmaps and free's page-tables from the index <start> to <start> + <count>.
 *
 * @param start the index of the page-table to start with
 * @param count the number of page-tables to remove
 */
void paging_unmapPageTables(u32 start,u32 count);

/**
 * Clones the current page-directory.
 *
 * @param stackFrame will contain the stack-frame for each thread after the call
 * @param newProc the process for which to create the page-dir
 * @return the frame-number of the new page-directory or 0 if there is not enough mem
 */
u32 paging_clonePageDir(u32 *stackFrames,sProc *newProc);

/**
 * Destroys the stack of the given thread. The function assumes that t is not the current
 * thread!
 *
 * @param t the thread to destroy
 */
void paging_destroyStacks(sThread *t);

/**
 * Destroyes the page-dir of the given process. That means all frames will be freed.
 * The function assumes that it is not the current process!
 *
 * @param p the process
 */
void paging_destroyPageDir(sProc *p);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

/**
 * Prints the user-part of the page-directory of the given process to the given buffer
 *
 * @param buffer the buffer
 * @param p the process
 */
void paging_sprintfVirtMem(sStringBuffer *buf,sProc *p);


#if DEBUGGING

/**
 * Prints all entries in the copy-on-write-list
 */
void paging_dbg_printCOW(void);

/**
 * @param virt the virtual address
 * @return the page-table-entry for the given address
 */
sPTEntry *paging_dbg_getPTEntry(u32 virt);

/**
 * Counts the number of pages that are currently present in the given page-directory
 *
 * @param pdir the page-directory
 * @return the number of pages
 */
u32 paging_dbg_getPageCount(void);

/**
 * Checks wether the given page-table is empty
 *
 * @param pt the pointer to the first entry of the page-table
 * @return true if empty
 */
bool paging_dbg_isPTEmpty(sPTEntry *pt);

/**
 * Counts the number of present pages in the given page-table
 *
 * @param pt the page-table
 * @return the number of present pages
 */
u32 paging_dbg_getPTEntryCount(sPTEntry *pt);

/**
 * Prints the given parts from the own page-directory
 *
 * @param parts the parts to print
 */
void paging_dbg_printOwnPageDir(u8 parts);

/**
 * Prints the given parts from the page-directory of the given process
 *
 * @param parts the parts to print
 */
void paging_dbg_printPageDirOf(sProc *p,u8 parts);

#endif

#endif /*PAGING_H_*/
