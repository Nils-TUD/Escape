/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#pragma once

#include <sys/common.h>
#include <sys/task/thread.h>

/**
 * Note that this module is intended to be used by the thread-module ONLY!
 */

/**
 * Inits the scheduler
 */
void sched_init(void);

/**
 * Adds the given thread as an idle-thread to the scheduler
 *
 * @param t the thread
 */
void sched_addIdleThread(sThread *t);

/**
 * Performs the scheduling. That means it picks the next thread to run and returns it
 *
 * @param old the current thread
 * @param runtime the runtime of the current thread in microseconds
 * @return the thread to run
 */
sThread *sched_perform(sThread *old,uint64_t runtime);

/**
 * Adjusts the priority of the given thread according to its used timeslice
 *
 * @param t the thread
 * @param threadCount total number of threads
 */
void sched_adjustPrio(sThread *t,size_t threadCount);

/**
 * Appends the given thread on the ready-queue and sets the state to ST_READY
 *
 * @param t the thread
 * @return true if successfull (i.e. if the thread is ready now and wasn't previously)
 */
bool sched_setReady(sThread *t);

/**
 * Puts the given thread to the beginning of the ready-queue
 *
 * @param t the thread
 * @return true if successfull (i.e. if the thread is ready now and wasn't previously)
 */
bool sched_setReadyQuick(sThread *t);

/**
 * Sets the thread in the blocked-state
 *
 * @param t the thread
 */
void sched_setBlocked(sThread *t);

/**
 * Suspends / resumes the given thread. That means it will be put into a state
 * that ensures that it can't be chosen until its resumed.
 *
 * @param t the thread
 * @param blocked whether to block or unblock it
 */
void sched_setSuspended(sThread *t,bool blocked);

/**
 * Removes the given thread from the scheduler (depending on the state)
 *
 * @param t the thread
 */
void sched_removeThread(sThread *t);

/**
 * Prints the status of the scheduler
 */
void sched_print(void);
