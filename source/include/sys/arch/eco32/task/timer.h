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
#include <sys/cpu.h>

class Timer : public TimerBase {
	friend class TimerBase;

	Timer() = delete;

	static const uintptr_t TIMER_BASE		= 0xF0000000;

	static const uint REG_CTRL				= 0;	/* timer control register */
	static const uint REG_DIVISOR			= 1;	/* timer divisor register */

	static const uint CTRL_EXP				= 0x01;	/* timer has expired */
	static const uint CTRL_IEN				= 0x02;	/* enable timer interrupt */

public:
	/**
	 * Acknoleges a timer-interrupt
	 */
	static void ackIntrpt();

	/**
	 * @return the elapsed milliseconds since the last timer-interrupt
	 */
	static time_t getTimeSinceIRQ();
};

inline void TimerBase::archInit() {
	uint *regs = (uint*)Timer::TIMER_BASE;
	/* set frequency */
	regs[Timer::REG_DIVISOR] = 1000 / Timer::FREQUENCY_DIV;
	/* enable timer */
	regs[Timer::REG_CTRL] = Timer::CTRL_IEN;
}

inline void Timer::ackIntrpt() {
	uint *regs = (uint*)TIMER_BASE;
	/* remove expired-flag */
	regs[REG_CTRL] = CTRL_IEN;
}

inline uint64_t TimerBase::cyclesToTime(uint64_t cycles) {
	return cycles / (CPU::getSpeed() / 1000000);
}

inline uint64_t TimerBase::timeToCycles(uint us) {
	return (CPU::getSpeed() / 1000000) * us;
}
