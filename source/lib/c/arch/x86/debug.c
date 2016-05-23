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
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/thread.h>

/* the x86-call instruction is 5 bytes long */
static const size_t CALL_INSTR_SIZE		= 5;

uintptr_t *getStackTrace(void) {
	static uintptr_t frames[20];
	uintptr_t end,start;
	uintptr_t *frame = &frames[0];
	ulong *bp;
	GET_REG(bp,bp);
	/* TODO just temporary */
	end = ((uintptr_t)bp + (MAX_STACK_PAGES * PAGE_SIZE - 1)) & ~(MAX_STACK_PAGES * PAGE_SIZE - 1);
	start = end - PAGE_SIZE * MAX_STACK_PAGES;

	for(size_t i = 0; i < ARRAY_SIZE(frames); i++) {
		/* prevent page-fault */
		if((uintptr_t)bp < start || (uintptr_t)bp >= end)
			break;
		*frame = *(bp + 1) - CALL_INSTR_SIZE;
		bp = (ulong*)*bp;
		frame++;
	}
	/* terminate */
	*frame = 0;
	return &frames[0];
}
