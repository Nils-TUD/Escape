/**
 * @version		$Id: debug.h 81 2008-11-23 12:43:27Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#include "common.h"

/**
 * Starts the timer
 */
void dbg_startTimer(void);

/**
 * Stops the timer and prints the number of clock-cycles done until startTimer()
 */
void dbg_stopTimer(const char *prefix);

#endif /*DEBUG_H_*/
