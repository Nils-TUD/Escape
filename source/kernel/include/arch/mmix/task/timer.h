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

#include <common.h>
#include <cpu.h>
#include <time.h>

class Timer : public TimerBase {
	friend class TimerBase;

	Timer() = delete;

	static const uintptr_t TIMER_BASE		= 0x8001000000000000;

	static const ulong REG_CTRL				= 0;	/* timer control register */
	static const ulong REG_DIVISOR			= 1;	/* timer divisor register */

	static const ulong CTRL_EXP				= 0x01;	/* timer has expired */
	static const ulong CTRL_IEN				= 0x02;	/* enable timer interrupt */

public:
	/**
	 * Acknoleges a timer-interrupt
	 */
	static void ackIntrpt();
};

inline void TimerBase::getTimeval(struct timeval *tv) {
	time_t time = Timer::getRuntime();
	tv->tv_sec = time / 1000;
	tv->tv_usec = time % 1000;
}

inline void TimerBase::archInit() {
	ulong *regs = (ulong*)Timer::TIMER_BASE;
	/* set frequency */
	regs[Timer::REG_DIVISOR] = 1000 / Timer::FREQUENCY_DIV;
	/* enable timer */
	regs[Timer::REG_CTRL] = Timer::CTRL_IEN;
}

inline void Timer::ackIntrpt() {
	ulong *regs = (ulong*)TIMER_BASE;
	/* remove expired-flag */
	regs[REG_CTRL] = CTRL_IEN;
}

inline uint64_t TimerBase::cyclesToTime(uint64_t cycles) {
	return cycles / (CPU::getSpeed() / 1000000);
}

inline uint64_t TimerBase::timeToCycles(uint us) {
	return (CPU::getSpeed() / 1000000) * us;
}
