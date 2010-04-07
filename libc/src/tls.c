/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/heap.h>
#include <esc/lock.h>
#include <string.h>
#include <assert.h>

static tULock tlsLock;
static u32 *tlsCopy = NULL;

/**
 * The TLS-system (variant 2) looks like this:
 *
 *                        %gs offset
 *                            |
 * TLS-region of a thread:    v
 * +--------+--------+--------+--------+
 * |  val3  |  val2  |  val1  |  TCB   | ---\
 * +--------+--------+--------+--------+    |
 *                   ^                      |
 *              /----/  ...      ...        |
 * dtv:         |        |        |         |
 * +--------+--------+--------+--------+    |
 * |  gen   |  ptr1  |  ptr2  |  ptr3  |    |
 * +--------+--------+--------+--------+    |
 * ^                                        |
 * \----------------------------------------/
 */

/* make gcc happy */
u32 init_tls(u32 entryPoint,u32 *tlsStart,u32 tlsSize);

u32 init_tls(u32 entryPoint,u32 *tlsStart,u32 tlsSize) {
	if(tlsSize) {
		u32 i,*dtv;
		locku(&tlsLock);
		/* create copy if not already done */
		if(tlsCopy == NULL) {
			tlsCopy = (u32*)malloc(tlsSize);
			assert(tlsCopy);
			memcpy(tlsCopy,tlsStart,tlsSize);
		}

		/* create dynamic thread vector for this thread */
		dtv = (u32*)malloc(tlsSize);
		assert(dtv);
		/* store size in first dword */
		dtv[0] = tlsSize;
		/* now put the pointers to the values into the array in the tls-region */
		tlsSize /= 4;
		for(i = 0; i < tlsSize - 1; i++) {
			dtv[i + 1] = (u32)(tlsCopy + i);
			/* put value in tls-region */
			tlsStart[i] = tlsCopy[i];
		}
		/* put pointer in thread-control-block */
		tlsStart[tlsSize - 1] = (u32)dtv;
		unlocku(&tlsLock);
	}
	return entryPoint;
}
