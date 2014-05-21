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

class Timer : public TimerBase {
	friend class TimerBase;

	Timer() = delete;

	static const uint64_t TOLERANCE			= 1000000;
	static const int MEASURE_COUNT			= 10;
	static const int REQUIRED_MATCHES		= 5;

public:
	/**
	 * Waits for <us> microseconds.
	 *
	 * @param ms the number of microseconds
	 */
	static void wait(uint us);

	/**
	 * Detects the CPU-speed
	 *
	 * @param busHz will be set to the bus frequency
	 * @return the speed in Hz
	 */
	static uint64_t detectCPUSpeed(uint64_t *busHz);

	/**
	 * Starts the timer
	 */
	static void start(bool isBSP);

private:
	static uint64_t determineSpeed(int instrCount,uint64_t *busHz);

	static uint64_t cpuMhz;
};

inline void TimerBase::archInit() {
}

inline uint64_t TimerBase::cyclesToTime(uint64_t cycles) {
	return cycles / Timer::cpuMhz;
}

inline uint64_t TimerBase::timeToCycles(uint us) {
	return Timer::cpuMhz * us;
}
