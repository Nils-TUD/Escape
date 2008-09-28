/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "common.h"
#include "paging.h"

/**
 * Prints the given page-directory
 * 
 * @param pagedir the page-directory
 */
void dbg_printPageDir(tPDEntry *pagedir);

/**
 * Prints the given page-table
 * 
 * @param no the number of the page-table
 * @param frame the frame of the page-table
 * @param pagetable the page-table
 */
void dbg_printPageTable(u32 no,u32 frame,tPTEntry *pagetable);

/**
 * Prints the given page
 * 
 * @param page a pointer to a page-table-entry
 */
void dbg_printPage(tPTEntry *page);

#endif /*DEBUG_H_*/
