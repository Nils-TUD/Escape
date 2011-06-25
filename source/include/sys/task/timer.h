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

#include <sys/common.h>

#ifdef __i386__
#include <sys/arch/i586/task/timer.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/timer.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/timer.h>
#endif

/* timer frequency => time-slice = 5ms */
#define TIMER_FREQUENCY			200

/* the time that we give one process (60ms) */
#define PROC_TIMESLICE			((1000 / TIMER_FREQUENCY) * 12)

/**
 * Initializes the timer
 */
void timer_init(void);

/**
 * @return the number of timer-interrupts so far
 */
size_t timer_getIntrptCount(void);

/**
 * @return the kernel-internal timestamp; starts from zero, in milliseconds, increased by timer-irq
 */
time_t timer_getTimestamp(void);

/**
 * Puts the given thread to sleep for the given number of milliseconds
 *
 * @param tid the thread-id
 * @param msecs the number of milliseconds to wait
 * @return 0 on success
 */
int timer_sleepFor(tid_t tid,time_t msecs);

/**
 * Removes the given thread from the timer
 *
 * @param tid the thread-id
 */
void timer_removeThread(tid_t tid);

/**
 * Handles a timer-interrupt
 */
void timer_intrpt(void);

/**
 * Prints the timer-queue
 */
void timer_print(void);

#endif /* TIMER_H_ */
