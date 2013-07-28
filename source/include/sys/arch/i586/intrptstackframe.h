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

#include <esc/common.h>

/* the stack frame for the interrupt-handler */
struct IntrptStackFrame {
	/* stack-pointer before calling isr-handler */
	uint32_t esp;
	/* segment-registers */
	uint32_t es;
	uint32_t ds;
	uint32_t fs;
	uint32_t gs;
	/* general purpose registers */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t : 32; /* esp from pusha */
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	/* interrupt-number */
	uint32_t intrptNo;
	/* error-code (for exceptions); default = 0 */
	uint32_t errorCode;
	/* pushed by the CPU */
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	/* if we come from user-mode this fields will be present and will be restored with iret */
	uint32_t uesp;
	uint32_t uss;
	/* available when interrupting an vm86-task */
	uint16_t vm86es;
	uint16_t : 16;
	uint16_t vm86ds;
	uint16_t : 16;
	uint16_t vm86fs;
	uint16_t : 16;
	uint16_t vm86gs;
	uint16_t : 16;
} A_PACKED;
