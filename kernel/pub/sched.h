/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef SCHED_H_
#define SCHED_H_

#include "../pub/common.h"
#include "../pub/proc.h"

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
 * Prints the ready-queue
 */
void sched_printReadyQueue(void);

/**
 * Enqueues the given process on the ready-queue
 *
 * @param p the process
 */
void sched_enqueueReady(sProc *p);

/**
 * Removes the first process from the ready-queue and returns it
 *
 * @return the process or NULL if there is no process
 */
sProc *sched_dequeueReady(void);

/**
 * Removes the given process from the queue
 *
 * @param p the process
 * @return true if the process has been removed
 */
bool sched_dequeueProc(sProc *p);

#endif /* SCHED_H_ */
