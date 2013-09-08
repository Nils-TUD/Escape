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

class CPU : public CPUBase {
	CPU() = delete;

public:
	/**
	 * Sets the given CPU-speed (no, the hardware is not changed ;))
	 *
	 * @param hz the CPU-speed in Hz
	 */
	static void setSpeed(uint32_t hz) {
		cpuHz = hz;
	}

	/**
	 * @return the pagefault-address
	 */
	static uint getBadAddr() asm("cpu_getBadAddr");
	static uint64_t cpuHz;
};

inline uint64_t CPUBase::rdtsc() {
	uint32_t upper, lower;
	asm volatile (
		"mvfs	%0,5\n"
		"mvfs	%1,6\n"
		: "=r"(upper), "=r"(lower)
	);
	return ((uint64_t)upper << 32) | lower;
}

inline uint64_t CPUBase::getSpeed() {
	return CPU::cpuHz;
}
