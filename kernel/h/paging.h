/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PAGING_H_
#define PAGING_H_

#include "common.h"
#include "mm.h"

/*#define TEST_PAGING*/

/**
 * Virtual memory layout:
 * 0x00000000: -------------------------------------
 *             |                                   |
 *             |               code                |
 *             |                                   |
 *             -------------------------------------
 *             |                                   |
 *      |      |               data                |
 *      v      |                                   |
 *             -------------------------------------
 *             |                ...                |
 *             -------------------------------------
 *      ^      |                                   |
 *      |      |               stack               |
 *             |                                   |
 * 0xC0000000: -------------------------------------
 *             |         kernel code+data          |
 *             -------------------------------------
 *      |      |             mm-stack              |
 *      v      -------------------------------------
 *             |                ...                |
 * 0xC0FFD000: -------------------------------------
 *             |       mapped temp page-table      |
 * 0xC0FFE000: -------------------------------------
 *             |        mapped temp page-dir       |
 * 0xC0FFF000: -------------------------------------
 *             |          mapped page-dir          |
 * 0xC1000000: -------------------------------------
 *             |        mapped page-tables         |
 * 0xC1400000: -------------------------------------
 *             |     temp mapped page-tables       |
 * 0xC1800000: -------------------------------------
 *             |                ...                |
 * 0xFFFFFFFF: -------------------------------------
 */

/* the virtual address of the kernel-area */
#define KERNEL_AREA_V_ADDR	((u32)0xC0000000)
/* the virtual address of the kernel itself */
#define KERNEL_V_ADDR		(KERNEL_AREA_V_ADDR + KERNEL_P_ADDR)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT		(PAGE_SIZE / 4)

/* the start of the mapped page-tables area */
#define MAPPED_PTS_START	(KERNEL_AREA_V_ADDR + 0x1000000)
/* the start of the temporary mapped page-tables area */
#define TMPMAP_PTS_START	(MAPPED_PTS_START + (PT_ENTRY_COUNT * PAGE_SIZE))

/* page-directories in virtual memory */
#define PAGE_DIR_AREA		(MAPPED_PTS_START - PAGE_SIZE)
/* needed for building a new page-dir */
#define PAGE_DIR_TMP_AREA	(PAGE_DIR_AREA - PAGE_SIZE)
/* area for a page-table */
#define PAGE_TABLE_AREA		(PAGE_DIR_TMP_AREA - PAGE_SIZE)

/* flags for paging_map() */
#define PG_WRITABLE		1
#define PG_SUPERVISOR	2
#define PG_COPYONWRITE	4

/* converts a virtual address to the page-directory-index for that address */
#define ADDR_TO_PDINDEX(addr) ((u32)(addr) / PAGE_SIZE / PT_ENTRY_COUNT)

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + ((u32)(addr) / PT_ENTRY_COUNT))

/* converts a virtual address to the index in the corresponding page-table */
#define ADDR_TO_PTINDEX(addr) (((u32)(addr) / PAGE_SIZE) % PT_ENTRY_COUNT)

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
} tPDEntry;

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
} tPTEntry;

/* the page-directory for the first process */
extern tPDEntry proc0PD[];

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Assembler routine to flush the TLB
 */
extern void paging_flushTLB(void);

/**
 * Assembler routine to exchange the page-directory to the given one
 * 
 * @param physAddr the physical address of the page-directory
 */
extern void paging_exchangePDir(u32 physAddr);

/**
 * Maps the page-table for the given virtual address to <frame> in the mapped
 * page-tables area.
 * 
 * @param pt the page-table for the mapping
 * @param virtual the virtual address
 * @param frame the frame-number
 * @param flush flush the TLB?
 */
void paging_mapPageTable(tPTEntry *pt,u32 virtual,u32 frame,bool flush);

/**
 * Unmaps the page-table for the given virtual address to <frame> out of the mapped
 * page-tables area.
 * 
 * @param pt the page-table for the mapping
 * @param virtual the virtual address
 * @param flush flush the TLB?
 */
void paging_unmapPageTable(tPTEntry *pt,u32 virtual,bool flush);

/**
 * Counts the number of pages that are currently present in the given page-directory
 * 
 * @param pdir the page-directory
 * @return the number of pages
 */
u32 paging_getPageCount(void);

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
 * page-dir!)
 * Note that the function will NOT flush the TLB!
 * 
 * @panic if there is not enough memory to get a frame for a page-table
 * 
 * @param virtual the virtual start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use.
 * 	a NULL-value causes the function to request MM_DEF-frames from mm on its own!
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 */
void paging_map(u32 virtual,u32 *frames,u32 count,u8 flags);

/**
 * Removes <count> pages starting at <virtual> from the page-directory and page-tables (in the
 * CURRENT page-dir!).
 * Note that the function will NOT free the frames and that it will NOT flush the TLB!
 * 
 * @param virtual the virtual start-address
 * @param count the number of pages to unmap
 */
void paging_unmap(u32 virtual,u32 count);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

#ifdef TEST_PAGING
void test_paging(void);
#endif

#endif /*PAGING_H_*/
