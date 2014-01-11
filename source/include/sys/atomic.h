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

class Atomic {
	Atomic() = delete;

public:
	/**
	 * Adds <value> to *<ptr> and returns the old value
	 */
	template<typename T, typename Y>
	static T add(T volatile *ptr, Y value);

	/**
	 * Compare and swap
	 */
	template<typename T, typename Y>
	static bool cmpnswap(T volatile *ptr, Y oldval, Y newval);
};

#ifdef __i386__
#include <sys/arch/i586/atomic.h>
#endif
#ifdef __eco32__
#include <sys/arch/eco32/atomic.h>
#endif
#ifdef __mmix__
#include <sys/arch/mmix/atomic.h>
#endif
