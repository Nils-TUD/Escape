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
 */
void dbg_printPageDir(void);

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
