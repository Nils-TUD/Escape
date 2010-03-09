/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SCHED_H_
#define SCHED_H_

#include <common.h>
#include <task/thread.h>

/**
 * Inits the scheduler
 */
void sched_init(void);

/**
 * Performs the scheduling. That means it picks the next thread to run and returns it
 *
 * @return the thread to run
 */
sThread *sched_perform(void);

/**
 * Chooses the given thread for running (removes it from a list, if necessary, and changes the state)
 *
 * @param t the thread
 */
void sched_setRunning(sThread *t);

/**
 * Enqueues the given thread on the ready-queue and sets the state to ST_READY
 *
 * @param t the thread
 */
void sched_setReady(sThread *t);

/**
 * Puts the given thread to the beginning of the ready-queue and sets the state to ST_READY
 *
 * @param t the thread
 */
void sched_setReadyQuick(sThread *t);

/**
 * Sets the thread in the blocked-state
 *
 * @param t the thread
 */
void sched_setBlocked(sThread *t);

/**
 * Unblocks all blocked threads that are waiting for the given event
 *
 * @param mask the mask that has to match
 * @param event the event
 */
void sched_unblockAll(u16 mask,u16 event);

/**
 * Blocks or unblocks the given thread for swapping. That means it will be put into a state
 * that ensures that it can't be chosen until swapping is done.
 *
 * @param t the thread
 * @param blocked wether to block or unblock it
 */
void sched_setBlockForSwap(sThread *t,bool blocked);

/**
 * Removes the given thread from the scheduler (depending on the state)
 *
 * @param t the thread
 */
void sched_removeThread(sThread *t);

#if DEBUGGING

/**
 * Prints the status of the scheduler
 */
void sched_dbg_print(void);

#endif

#endif /* SCHED_H_ */
