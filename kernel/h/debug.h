/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "common.h"
#include "paging.h"
#include "proc.h"

/**
 * Starts the timer
 */
void dbg_startTimer(void);

/**
 * Stops the timer and prints the number of clock-cycles done until startTimer()
 */
void dbg_stopTimer(void);

/**
 * Prints the given process
 *
 * @param p the pointer to the process
 */
void dbg_printProcess(tProc *p);

/**
 * Prints the given process-state
 *
 * @param state the pointer to the state-struct
 */
void dbg_printProcessState(tProcSave *state);

/**
 * Prints the current page-directory
 *
 * @param includeKernel wether the kernel-page-table should be printed
 */
void dbg_printPageDir(bool includeKernel);

/**
 * Prints the user-space page-directory
 */
void dbg_printUserPageDir(void);

/**
 * Prints the given page-table
 *
 * @param no the number of the page-table
 * @param pde the page-dir-entry
 */
void dbg_printPageTable(u32 no,sPDEntry *pde);

/**
 * Prints the given page
 *
 * @param page a pointer to a page-table-entry
 */
void dbg_printPage(sPTEntry *page);

#endif /*DEBUG_H_*/
