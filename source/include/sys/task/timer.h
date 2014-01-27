/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

class OStream;

class TimerBase {
	TimerBase() = delete;

	/* an entry in the listener-list */
	struct Listener {
		tid_t tid;
		/* difference to the previous listener */
		time_t time;
		/* if true, the thread is blocked during that time. otherwise it can run and will not be waked
		 * up, but gets a signal (SIG_ALARM) */
		bool block;
		Listener *next;
	};

	struct PerCPU {
		/* total elapsed milliseconds */
		time_t elapsedMsecs;
		time_t lastResched;
		size_t timerIntrpts;
	};

	static const size_t LISTENER_COUNT		= 1024;

public:
	/* timer period = 5ms */
	static const unsigned FREQUENCY_DIV		= 200;
	/* time-slice for a thread (60ms) */
	static const unsigned TIMESLICE			= ((1000 / FREQUENCY_DIV) * 4);

	/**
	 * Initializes the timer
	 */
	static void init();

	/**
	 * @return the number of timer-interrupts so far
	 */
	static size_t getIntrptCount() {
		return perCPU[0].timerIntrpts;
	}

	/**
	 * @return the kernel-internal timestamp; starts from zero, in milliseconds, increased by timer-irq
	 */
	static time_t getTimestamp() {
		return perCPU[0].elapsedMsecs;
	}

	/**
	 * @param cycles the number of cycles
	 * @return the number of microseconds
	 */
	static uint64_t cyclesToTime(uint64_t cycles);

	/**
	 * @param us the number of microseconds
	 * @return the number of cycles
	 */
	static uint64_t timeToCycles(uint us);

	/**
	 * Puts the given thread to sleep for the given number of milliseconds
	 *
	 * @param tid the thread-id
	 * @param msecs the number of milliseconds to wait
	 * @param block whether to block the thread or not (if so, it will be waked up, otherwise it gets
	 *  SIG_ALARM)
	 * @return 0 on success
	 */
	static int sleepFor(tid_t tid,time_t msecs,bool block);

	/**
	 * Removes the given thread from the timer
	 *
	 * @param tid the thread-id
	 */
	static void removeThread(tid_t tid);

	/**
	 * Handles a timer-interrupt
	 *
	 * @return true if we should perform a thread-switch
	 */
	static bool intrpt();

	/**
	 * Prints the timer-queue
	 *
	 * @param os the output-stream
	 */
	static void print(OStream &os);

private:
	/**
	 * Inits the architecture-dependent part of the timer
	 */
	static void archInit();

	static klock_t lock;
	static PerCPU *perCPU;
	static time_t lastRuntimeUpdate;
	static Listener listenObjs[LISTENER_COUNT];
	static Listener *freeList;
	/* processes that should be waked up to a specified time */
	static Listener *listener;
};

#ifdef __i386__
#include <sys/arch/i586/task/timer.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/task/timer.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/task/timer.h>
#endif
