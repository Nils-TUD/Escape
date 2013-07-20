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
#include <sys/printf.h>

class CPUBase {
	CPUBase() = delete;

public:
	/**
	 * @return the timestamp-counter value
	 */
	static uint64_t rdtsc();

	/**
	 * @return the speed of the CPU in Hz
	 */
	static uint64_t getSpeed();

	/**
	 * Prints information about the used CPU into the given string-buffer
	 *
	 * @param buf the string-buffer
	 */
	static void sprintf(sStringBuffer *buf);

	/**
	 * Prints the CPU-information
	 */
	static void print();
};

#ifdef __i386__
#include <sys/arch/i586/cpu.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/cpu.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/cpu.h>
#endif
