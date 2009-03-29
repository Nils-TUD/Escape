/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "common.h"

#define TIMER_FREQUENCY			50

/* the time that we give one process */
#define PROC_TIMESLICE			((1000 / TIMER_FREQUENCY) * 3)

/**
 * Initializes the timer
 */
void timer_init(void);

/**
 * Puts the given process to sleep for the given number of milliseconds
 *
 * @param pid the process-id
 * @param msecs the number of milliseconds to wait
 * @return 0 on success
 */
s32 timer_sleepFor(tPid pid,u32 msecs);

/**
 * Handles a timer-interrupt
 */
void timer_intrpt(void);

#endif /* TIMER_H_ */
