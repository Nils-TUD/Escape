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

#include <esc/common.h>

class Timer : public TimerBase {
	friend class TimerBase;

	Timer() = delete;

	static const uint64_t TOLERANCE			= 1000000;
	static const int MEASURE_COUNT			= 10;
	static const int REQUIRED_MATCHES		= 5;

	static const uint BASE_FREQUENCY		= 1193182;
	static const uint IOPORT_CTRL 			= 0x43;
	static const uint IOPORT_CHAN0DIV 		= 0x40;

	/* counter to select */
	static const uint CTRL_CHAN0			= 0x00;
	static const uint CTRL_CHAN1			= 0x40;
	static const uint CTRL_CHAN2			= 0x80;
	/* read/write mode */
	static const uint CTRL_RWLO				= 0x10;	/* low byte only */
	static const uint CTRL_RWHI				= 0x20;	/* high byte only */
	static const uint CTRL_RWLOHI			= 0x30;	/* low byte first, then high byte */
	/* mode */
	static const uint CTRL_MODE1 			= 0x02;	/* programmable one shot */
	static const uint CTRL_MODE2 			= 0x04;	/* rate generator */
	static const uint CTRL_MODE3 			= 0x06;	/* square wave generator */
	static const uint CTRL_MODE4 			= 0x08;	/* software triggered strobe */
	static const uint CTRL_MODE5 			= 0x0A;	/* hardware triggered strobe */
	/* count mode */
	static const uint CTRL_CNTBIN16 		= 0x00;	/* binary 16 bit */
	static const uint CTRL_CNTBCD 			= 0x01;	/* BCD */

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
	 * @return the speed in Hz
	 */
	static uint64_t detectCPUSpeed();

private:
	static uint64_t determineSpeed(int instrCount);

	static uint64_t cpuMhz;
};

inline uint64_t TimerBase::cyclesToTime(uint64_t cycles) {
	return cycles / Timer::cpuMhz;
}

inline uint64_t TimerBase::timeToCycles(uint us) {
	return Timer::cpuMhz * us;
}
