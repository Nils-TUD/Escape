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

static inline bool atomic_cmpnswap(long volatile *ptr,long oldval,long newval) {
	__asm__ volatile ("PUT	rP,%0\n" : : "r"(oldval));
	__asm__ volatile (
		"CSWAP	%0,%1,0\n"
		: "+r"(newval)
		: "r"(ptr)
		: "memory"
	);
	return newval == 1;
}

static inline long atomic_add(long volatile *ptr,long value) {
	long old = *ptr;
	while(!atomic_cmpnswap(ptr,old,old + value))
		old = *ptr;
	return old;
}
