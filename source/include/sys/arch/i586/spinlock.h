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
#include <sys/arch/i586/atomic.h>
#include <sys/cpu.h>

inline void SpinLock::acquire(klock_t *l) {
    while(!Atomic::cmpnswap(l, 0, 1))
        CPU::pause();
}

inline bool SpinLock::tryAcquire(klock_t *l) {
	return Atomic::cmpnswap(l, 0, 1);
}

inline void SpinLock::release(klock_t *l) {
	asm volatile ("movl	$0,%0" : : "m"(*l) : "memory");
}
