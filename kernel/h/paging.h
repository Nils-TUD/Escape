/**
 * @version		$Id: paging.h 88 2008-11-24 14:36:16Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PAGING_H_
#define PAGING_H_

#include "common.h"
#include "mm.h"
#include "proc.h"

/**
 * Virtual memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |             user-code             |     |
 *             |                                   |
 *             +-----------------------------------+     u
 *             |                                   |     s
 *      |      |             user-data             |     e
 *      v      |                                   |     r
 *             +-----------------------------------+     a
 *             |                ...                |     r
 *             +-----------------------------------+     e
 *      ^      |                                   |     a
 *      |      |            user-stack             |
 *             |                                   |     |
 * 0xC0000000: +-----------------------------------+   -----
 *             |                ...                |     |
 * 0xC0100000: +-----------------------------------+     |
 *             |         kernel code+data          |     |
 *             +-----------------------------------+
 *      |      |             mm-stack              |     k
 *      v      +-----------------------------------+     e
 *             |                ...                |     r
 * 0xC03FE000: +-----------------------------------+     n
 *             |        mapped temp page-dir       |     e
 * 0xC03FF000: +-----------------------------------+     l
 *             |          mapped page-dir          |     a
 * 0xC0400000: +-----------------------------------+     r
 *      ^      |                                   |     e
 *      |      |            kernel-heap            |     a
 *             |                                   |
 * 0xC1800000: +-----------------------------------+     |
 *             |                ...                |     |
 * 0xFF400000: +-----------------------------------+     |      -----
 *             |     temp mapped page-tables       |     |        |
 * 0xFF800000: +-----------------------------------+     |        |
 *             |                ...                |     |        |
 * 0xFFBFE000: +-----------------------------------+     |
 *             |         temp kernel-stack         |     |     not shared page-tables (3)
 * 0xFFBFF000: +-----------------------------------+     |
 *             |            kernel-stack           |     |        |
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
#define TMPMAP_PTS_START	(MAPPED_PTS_START - (PT_ENTRY_COUNT * PAGE_SIZE * 2))
/* the start of the kernel-heap */
#define KERNEL_HEAP_START	(KERNEL_AREA_V_ADDR + (PT_ENTRY_COUNT * PAGE_SIZE))
/* the size of the kernel-heap (16 MiB) */
#define KERNEL_HEAP_SIZE	(PT_ENTRY_COUNT * PAGE_SIZE/* * 4*/)

/* page-directories in virtual memory */
#define PAGE_DIR_AREA		(KERNEL_HEAP_START - PAGE_SIZE)
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA	(PAGE_DIR_AREA - PAGE_SIZE)
/* our kernel-stack */
#define KERNEL_STACK		(MAPPED_PTS_START - PAGE_SIZE)
/* temporary stack for cloning the stack */
#define KERNEL_STACK_TMP	(KERNEL_STACK - PAGE_SIZE)

/* the size of the temporary stack we use at the beginning */
#define TMP_STACK_SIZE		PAGE_SIZE

/* flags for paging_map() */
#define PG_WRITABLE			1
#define PG_SUPERVISOR		2
#define PG_COPYONWRITE		4
/* tells paging_map() that it gets the frame-address and should convert it to a frame-number first */
#define PG_ADDR_TO_FRAME	8

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr) ((u32)(addr) / PAGE_SIZE / PT_ENTRY_COUNT)

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr) (((u32)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT)

/* converts pages to page-tables (how many page-tables are required for the pages?) */
#define PAGES_TO_PTS(pageCount) (((pageCount) + (PT_ENTRY_COUNT - 1)) / PT_ENTRY_COUNT)

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
						: 2,
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
 * Ensures that the current page-dir is mapped and can be accessed at PAGE_DIR_AREA
 */
void paging_mapPageDir(void);

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
 * @param virtual the virtual address
 * @return true if so
 */
bool paging_isMapped(u32 virtual);

/**
 * Checks wether the given address-range is currently readable for the user
 *
 * @param virtual the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserReadable(u32 virtual,u32 count);

/**
 * Checks wether the given address-range is currently readable
 *
 * @param virtual the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeReadable(u32 virtual,u32 count);

/**
 * Checks wether the given address-range is currently writable for the user.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virtual the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeUserWritable(u32 virtual,u32 count);

/**
 * Checks wether the given address-range is currently writable.
 * Note that the function handles copy-on-write if necessary. So you can be sure that you
 * can write to the page(s) after calling the function.
 *
 * @param virtual the start-address
 * @param count the number of bytes
 * @return true if so
 */
bool paging_isRangeWritable(u32 virtual,u32 count);

/**
 * Determines the frame-number for the given virtual-address. This should not be used
 * for user-space addresses!
 *
 * @param virtual the virtual address
 * @return the frame-number to which it is currently mapped or 0 if not mapped
 */
u32 paging_getFrameOf(u32 virtual);

/**
 * Determines how many new frames we need for calling paging_map(<virtual>,...,<count>,...).
 *
 * @param virtual the virtual start-address
 * @param count the number of pages to map
 * @return the number of new frames we would need
 */
u32 paging_countFramesForMap(u32 virtual,u32 count);

/**
 * Maps <count> virtual addresses starting at <virtual> to the given frames (in the CURRENT
 * page-dir!). You can decide (via <force>) wether the mapping should be done in every
 * case or just if the page is not already mapped.
 * Note that the function will NOT flush the TLB!
 *
 * @panic if there is not enough memory to get a frame for a page-table
 *
 * @param virtual the virtual start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 * @param force wether the mapping should be overwritten
 */
void paging_map(u32 virtual,u32 *frames,u32 count,u8 flags,bool force);

/**
 * Removes <count> pages starting at <virtual> from the page-tables (in the CURRENT page-dir!).
 * If you like the function free's the frames.
 * Note that the function will NOT flush the TLB and will not delete page-tables!
 *
 * @param virtual the virtual start-address
 * @param count the number of pages to unmap
 * @param freeFrames wether the frames should be free'd and not just unmapped
 */
void paging_unmap(u32 virtual,u32 count,bool freeFrames);

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
 * @param stackFrame will contain the stack-frame after the call
 * @param newProc the process for which to create the page-dir
 * @return the frame-number of the new page-directory or 0 if there is not enough mem
 */
u32 paging_clonePageDir(u32 *stackFrame,sProc *newProc);

/**
 * Destroyes the page-dir of the given process. That means all frames will be freed.
 *
 * @param p the process
 */
void paging_destroyPageDir(sProc *p);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

#if DEBUGGING

/**
 * Prints all entries in the copy-on-write-list
 */
void paging_dbg_printCOW(void);

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
 * Prints the page-directory part that is occupied by the heap
 */
void paging_dbg_printHeap(void);

/**
 * Prints the current page-directory
 *
 * @param includeKernel wether the kernel-page-table should be printed
 */
void paging_dbg_printPageDir(bool includeKernel);

/**
 * Prints the user-space page-directory
 */
void paging_dbg_printUserPageDir(void);

/**
 * Prints the given page-table
 *
 * @param no the number of the page-table
 * @param pde the page-dir-entry
 */
void paging_dbg_printPageTable(u32 no,sPDEntry *pde);

/**
 * Prints the given page
 *
 * @param page a pointer to a page-table-entry
 */
void paging_dbg_printPage(sPTEntry *page);

#endif

#endif /*PAGING_H_*/
