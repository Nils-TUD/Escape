#ifndef PAGING_H_
#define PAGING_H_

#include "common.h"
#include "mm.h"

/* the virtual address of the kernel-area */
#define KERNEL_AREA_V_ADDR	((u32)0xC0000000)
/* the virtual address of the kernel itself */
#define KERNEL_V_ADDR		(KERNEL_AREA_V_ADDR + KERNEL_P_ADDR)

/* the number of entries in a page-directory or page-table */
#define PT_ENTRY_COUNT		(PAGE_SIZE / 4)

/* flags for paging_map() */
#define PG_WRITABLE		1
#define PG_SUPERVISOR	2

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
	/* enabled if the page-table has been accessed */
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
	ptAddress			: 20;
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
	/* enabled if the page has been accessed */
	accessed			: 1,
	/* enabled if the page can not be removed currently */
	dirty				: 1,
	/* 1 ignored bit */
						: 1,
	/* The Global, or 'G' above, flag, if set, prevents the TLB from updating the address in
	 * it's cache if CR3 is reset. Note, that the page global enable bit in CR4 must be set
	 * to enable this feature. */
	global				: 1,
	/* can be used by the OS */
						: 3,
	/* the physical address of the page */
	physAddress			: 20;
} tPTEntry;

/* the page-directory for the first process */
extern tPDEntry proc0PD[];

/**
 * Inits the paging. Sets up the page-dir and page-tables for the kernel and enables paging
 */
void paging_init(void);

/**
 * Maps <count> virtual addresses starting at <virtual> to the given frames
 * 
 * @panic if there is not enough memory to get a frame for a page-table
 * 
 * @param pdir the page-directory
 * @param virtual the virtual start-address
 * @param frames an array with <count> elements which contains the frame-numbers to use
 * @param count the number of pages to map
 * @param flags some flags for the pages (PG_*)
 */
void paging_map(tPDEntry *pdir,s8 *virtual,u32 *frames,u32 count,u8 flags);

/**
 * Unmaps the page-table 0. This should be used only by the GDT to unmap the first page-table as
 * soon as the GDT is setup for a flat memory layout!
 */
void paging_gdtFinished(void);

/**
 * Removes <count> pages starting at <virtual> from the page-directory and page-tables.
 * Note that the function will NOT free the frames!
 * 
 * @param pdir the page-directory
 * @param virtual the virtual start-address
 * @param count the number of pages to unmap
 */
void paging_unmap(tPDEntry *pdir,s8 *virtual,u32 count);

#endif /*PAGING_H_*/
