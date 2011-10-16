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

#ifndef KLOCK_H_
#define KLOCK_H_

#include <sys/common.h>
#include <sys/util.h>
#include <sys/log.h>

#ifdef __i386__
static A_INLINE void spinlock_aquire(klock_t *l) {
	__asm__ (
		"mov	$1,%%ecx;"
		"1:"
		"	xor		%%eax,%%eax;"
		"	lock	cmpxchg %%ecx,(%0);"
		"	jz		2f;"
		/* improves the performance and lowers the power-consumption of spinlocks */
		"	pause;"
		"	jmp		1b;"
		"2:;"
		/* outputs */
		:
		/* inputs */
		: "D" (l)
		/* eax, ecx and cc will be clobbered; we need memory as well because *l is changed */
		: "eax", "ecx", "cc", "memory"
	);
}

static A_INLINE void spinlock_release(klock_t *l) {
	*l = 0;
}
#endif
/* eco32 and mmix do not support smp */
#ifdef __eco32__
#define spinlock_aquire(l)			(void)(l)
#define spinlock_release(l)		(void)(l)
#endif
#ifdef __mmix__
#define spinlock_aquire(l)			(void)(l)
#define spinlock_release(l)		(void)(l)
#endif

#endif /* KLOCK_H_ */
