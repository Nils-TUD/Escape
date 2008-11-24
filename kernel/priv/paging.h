/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef PRIVPAGING_H_
#define PRIVPAGING_H_

#include "../pub/common.h"
#include "../pub/paging.h"

/* builds the address of the page in the mapped page-tables to which the given addr belongs */
#define ADDR_TO_MAPPED(addr) (MAPPED_PTS_START + (((u32)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))
#define ADDR_TO_MAPPED_CUSTOM(mappingArea,addr) ((mappingArea) + \
		(((u32)(addr) & ~(PAGE_SIZE - 1)) / PT_ENTRY_COUNT))

/* converts the given virtual address to a physical
 * (this assumes that the kernel lies at 0xC0000000) */
#define VIRT2PHYS(addr) ((u32)(addr) + 0x40000000)

/* the page-directory for process 0 */
sPDEntry proc0PD[PAGE_SIZE / sizeof(sPDEntry)] __attribute__ ((aligned (PAGE_SIZE)));
/* the page-table for process 0 */
sPTEntry proc0PT[PAGE_SIZE / sizeof(sPTEntry)] __attribute__ ((aligned (PAGE_SIZE)));

/* copy-on-write */
typedef struct {
	u32 frameNumber;
	sProc *proc;
} sCOW;

/**
 * Assembler routine to enable paging
 */
extern void paging_enable(void);

/**
 * Checks wether the given page-table is empty
 *
 * @param pt the pointer to the first entry of the page-table
 * @return true if empty
 */
static bool paging_isPTEmpty(sPTEntry *pt);

/**
 * Counts the number of present pages in the given page-table
 *
 * @param pt the page-table
 * @return the number of present pages
 */
static u32 paging_getPTEntryCount(sPTEntry *pt);

/**
 * paging_map() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 * @param mappingArea the address of the mapping area to use
 */
static void paging_mapintern(u32 pageDir,u32 mappingArea,u32 virtual,u32 *frames,u32 count,u8 flags,
		bool force);

/**
 * paging_unmap() for internal usages
 *
 * @param mappingArea the address of the mapping area to use
 */
static void paging_unmapIntern(u32 mappingArea,u32 virtual,u32 count,bool freeFrames);

/**
 * paging_unmapPageTables() for internal usages
 *
 * @param pageDir the address of the page-directory to use
 */
static void paging_unmapPageTablesIntern(u32 pageDir,u32 start,u32 count);

/**
 * Helper function to put the given frames for the given new process and the current one
 * in the copy-on-write list and mark the pages for the parent as copy-on-write if not
 * already done.
 *
 * @panic not enough mem for linked-list nodes and entries
 *
 * @param virtual the virtual address to map
 * @param frames the frames array
 * @param count the number of pages to map
 * @param newProc the new process
 */
static void paging_setCOW(u32 virtual,u32 *frames,u32 count,sProc *newProc);

/**
 * Prints all entries in the copy-on-write-list
 */
static void paging_printCOW(void);


#endif /* PRIVPAGING_H_ */
