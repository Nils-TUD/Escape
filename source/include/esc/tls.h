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

#include <esc/common.h>
#include <assert.h>

#if defined(__x86__)
#	include <esc/arch/x86/tls.h>
#elif defined(__eco32__)
#	include <esc/arch/eco32/tls.h>
#elif defined(__mmix__)
#	include <esc/arch/mmix/tls.h>
#endif

#define MAX_TLS_ENTRIES		4

#if defined(__cplusplus)
extern "C" {
#endif

extern long __tls_num;

/**
 * Returns the value at index <idx> from thread local storage. It assumes that <idx> exists.
 *
 * @param idx the index
 * @return the value
 */
static inline ulong tlsget(size_t idx) {
	ulong *tls = *(ulong**)stack_top(2);
	assert(idx < (size_t)__tls_num);
	return tls[idx];
}

/**
 * Adds a value to TLS.
 *
 * @return the index for that value
 */
size_t tlsadd(void);

/**
 * Sets the value at index <idx> to <val>. It assumes that <idx> exists.
 *
 * @param idx the index
 * @param val the value
 */
static inline void tlsset(size_t idx,ulong val) {
	ulong *tls = *(ulong**)stack_top(2);
	assert(idx < (size_t)__tls_num);
	tls[idx] = val;
}

#if defined(__cplusplus)
}
#endif
