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

#ifndef TIMER_H_
#define TIMER_H_

#include <common.h>

#define TIMER_FREQUENCY			50

/* the time that we give one process */
#define PROC_TIMESLICE			((1000 / TIMER_FREQUENCY) * 3)

/**
 * Initializes the timer
 */
void timer_init(void);

/**
 * @return the number of timer-interrupts so far
 */
u32 timer_getIntrptCount(void);

/**
 * Puts the given thread to sleep for the given number of milliseconds
 *
 * @param tid the thread-id
 * @param msecs the number of milliseconds to wait
 * @return 0 on success
 */
s32 timer_sleepFor(tTid tid,u32 msecs);

/**
 * Removes the given thread from the timer
 *
 * @param tid the thread-id
 */
void timer_removeThread(tTid tid);

/**
 * Handles a timer-interrupt
 */
void timer_intrpt(void);

#if DEBUGGING

void timer_dbg_print(void);

#endif

#endif /* TIMER_H_ */
