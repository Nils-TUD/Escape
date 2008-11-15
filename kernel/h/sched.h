/**
 * @version		$Id: proc.h 48 2008-11-14 20:32:43Z nasmussen $
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
tProc *sched_perform(void);

/**
 * Prints the ready-queue
 */
void sched_printReadyQueue(void);

/**
 * Enqueues the given process on the ready-queue
 *
 * @param p the process
 */
void sched_enqueueReady(tProc *p);

/**
 * Removes the first process from the ready-queue and returns it
 *
 * @return the process or NULL if there is no process
 */
tProc *sched_dequeueReady(void);

/**
 * Removes the given process from the queue
 *
 * @param p the process
 */
void sched_dequeueProc(tProc *p);

#endif /* SCHED_H_ */
