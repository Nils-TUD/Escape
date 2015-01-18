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

#include <sys/arch.h>
#include <sys/atomic.h>
#include <sys/common.h>
#include <sys/conf.h>
#include <sys/tls.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

long __tls_num = 0;

void initTLS(void);

void initTLS(void) {
	ulong *tls = calloc(MAX_TLS_ENTRIES,sizeof(ulong));
	if(!tls)
		error("Not enough memory for TLS struct");

	ulong **ptr = (ulong**)stack_top(2);
	*ptr = tls;
}

size_t tlsadd(void) {
	long idx = atomic_add(&__tls_num,+1);
	if(idx >= MAX_TLS_ENTRIES)
		error("All TLS slots in use");
	return idx;
}
