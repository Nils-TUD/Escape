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
#include <esc/thread.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

static tULock tlsLock;
static u32 *tlsCopy = NULL;

/**
 * TODO: Actually this is not exact the model described in doc/thread-local-storage.pdf.
 * But linux seems to do it this way, dmd expects it to be that way (at least on linux) and it
 * works for gcc (c) and dmd (d). So its ok for now. But I don't know how it works with dynamic
 * linking...
 *
 * The TLS-system looks like this:
 *
 *                        %gs offset
 *                            |
 * TLS-region of a thread:    v
 * +--------+--------+--------+--------+
 * |  val3  |  val2  |  val1  |  TCB   | ---\
 * +--------+--------+--------+--------+    |
 *                            ^-------------/
 */

/* make gcc happy */
u32 init_tls(u32 entryPoint,u32 *tlsStart,u32 tlsSize);

u32 init_tls(u32 entryPoint,u32 *tlsStart,u32 tlsSize) {
	if(tlsSize) {
		u32 i;
		locku(&tlsLock);
		/* create copy if not already done */
		if(tlsCopy == NULL) {
			tlsCopy = (u32*)malloc(tlsSize);
			assert(tlsCopy);
			memcpy(tlsCopy,tlsStart,tlsSize);
		}

		/* copy values to TLS-region */
		tlsSize /= 4;
		for(i = 0; i < tlsSize - 1; i++)
			tlsStart[i] = tlsCopy[i];
		/* put pointer to TCB in TCB */
		tlsStart[tlsSize - 1] = (u32)(tlsStart + tlsSize - 1);
		unlocku(&tlsLock);
	}
	return entryPoint;
}
