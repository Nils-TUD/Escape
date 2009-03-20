/**
 * @version		$Id: sched.h 81 2008-11-23 12:43:27Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SCHED_H_
#define SCHED_H_

#include "../h/common.h"
#include "../h/proc.h"

/**
 * Inits the scheduler
 */
void sched_init(void);

/**
 * Performs the scheduling. That means it picks the next process to run and returns it
 *
 * @return the process to run
 */
sProc *sched_perform(void);

/**
 * Choses the given process for running
 *
 * @param p the process
 */
void sched_setRunning(sProc *p);

/**
 * Enqueues the given process on the ready-queue and sets the state to ST_READY
 *
 * @param p the process
 */
void sched_setReady(sProc *p);

/**
 * Sets the process in the blocked-state
 *
 * @param p the process
 */
void sched_setBlocked(sProc *p);

/**
 * Unblocks all blocked processes that are waiting for the given event
 */
void sched_unblockAll(u8 event);

/**
 * Removes the first process from the ready-queue and returns it
 *
 * @return the process or NULL if there is no process
 */
sProc *sched_dequeueReady(void);

/**
 * Removes the given process from the scheduler (depending on the state)
 *
 * @param p the process
 */
void sched_removeProc(sProc *p);

/**
 * Removes the given process from the ready-queue
 *
 * @param p the process
 * @return true if the process has been removed
 */
bool sched_dequeueReadyProc(sProc *p);

#if DEBUGGING

/**
 * Prints the status of the scheduler
 */
void sched_dbg_print(void);

#endif

#endif /* SCHED_H_ */
