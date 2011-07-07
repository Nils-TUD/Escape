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

#include <esc/common.h>
#include <esc/arch/i586/register.h>
#include <esc/debug.h>

#define MAX_STACK_PAGES		128
#define PAGE_SIZE			4096

#define MAX_STACK_DEPTH		20
/* the x86-call instruction is 5 bytes long */
#define CALL_INSTR_SIZE		5

uintptr_t *getStackTrace(void) {
	static uintptr_t frames[MAX_STACK_DEPTH];
	uintptr_t end,start;
	size_t i;
	uint32_t *ebp;
	uintptr_t *frame = &frames[0];
	GET_REG("ebp",ebp);
	/* TODO just temporary */
	end = ((uintptr_t)ebp + (MAX_STACK_PAGES * PAGE_SIZE - 1)) & ~(MAX_STACK_PAGES * PAGE_SIZE - 1);
	start = end - PAGE_SIZE * MAX_STACK_PAGES;

	for(i = 0; i < MAX_STACK_DEPTH; i++) {
		/* prevent page-fault */
		if((uintptr_t)ebp < start || (uintptr_t)ebp >= end)
			break;
		*frame = *(ebp + 1) - CALL_INSTR_SIZE;
		ebp = (uint32_t*)*ebp;
		frame++;
	}
	/* terminate */
	*frame = 0;
	return &frames[0];
}
