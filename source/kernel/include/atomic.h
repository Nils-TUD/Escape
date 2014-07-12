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

class Atomic {
	Atomic() = delete;

public:
	/**
	 * Adds <value> to *<ptr> and returns the old value
	 */
	template<typename T,typename Y>
	static T fetch_and_add(T volatile *ptr,Y value);

	/**
	 * ORs <value> to *<ptr> and returns the old value
	 */
	template<typename T,typename Y>
	static T fetch_and_or(T volatile *ptr,Y value);

	/**
	 * ANDs <value> to *<ptr> and returns the old value
	 */
	template<typename T,typename Y>
	static T fetch_and_and(T volatile *ptr,Y value);

	/**
	 * Compare and swap
	 */
	template<typename T,typename Y>
	static bool cmpnswap(T volatile *ptr,Y oldval,Y newval);
};

#if defined(__x86__)
#	include <arch/x86/atomic.h>
#elif defined(__eco32__)
#	include <arch/eco32/atomic.h>
#elif defined(__mmix__)
#	include <arch/mmix/atomic.h>
#endif
