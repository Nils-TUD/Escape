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

class Config {
	static const size_t MAX_BPNAME_LEN		= 16;
	static const size_t MAX_BPVAL_LEN		= 32;

public:
	enum {
		TIMER_FREQ		= 0,
		MAX_PROCS		= 1,
		MAX_FDS			= 2,
		LOG				= 3,
		LOG_TO_VGA		= 4,
		LINE_BY_LINE	= 5,
		CPU_COUNT		= 6,
		SMP				= 7,
		TICKS_PER_SEC	= 8,
		FORCE_PIT		= 9,
		FORCE_PIC		= 10,
		ACCURATE_CPU	= 11,
		ROOT_DEVICE		= 32,
		SWAP_DEVICE		= 33,
	};

	/**
	 * Parses the given boot-parameter and sets the configuration-options correspondingly
	 *
	 * @param argc the number of args
	 * @param argv the arguments
	 */
	static void parseBootParams(int argc,const char *const *argv);

	/**
	 * Returns the value for the given configuration-string-value
	 *
	 * @param id the config-id
	 * @return the value or NULL
	 */
	static const char *getStr(int id);

	/**
	 * Returns the value for the given configuration-value
	 *
	 * @param id the config-id
	 * @return the value or < 0 if an error occurred
	 */
	static long get(int id);

private:
	static void set(const char *name,const char *value);

	static uint32_t flags;
	static char rootDev[];
	static char swapDev[];
};
