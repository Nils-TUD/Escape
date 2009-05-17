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

#include "common.h"
#include "thread.h"

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
 * Choses the given thread for running
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
 */
void sched_unblockAll(u8 event);

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
