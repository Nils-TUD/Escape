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

#include <esc/arch.h>

.section .text

.global lookup_resolveStart
.extern lookup_resolve

lookup_resolveStart:
	push	%ARG_1
	push	%ARG_2
	push	%ARG_3
	push	%ARG_4
	push	%ARG_5
	push	%ARG_6
	mov		6 * WORDSIZE(%rsp),%ARG_1
	mov		7 * WORDSIZE(%rsp),%ARG_2
	call	lookup_resolve
	pop		%ARG_6
	pop		%ARG_5
	pop		%ARG_4
	pop		%ARG_3
	pop		%ARG_2
	pop		%ARG_1
	add		$(2 * WORDSIZE),%rsp
	jmp		*%rax
