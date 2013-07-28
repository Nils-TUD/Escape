/*
 * Copyright (C) 2013, Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * This file is part of NRE (NOVA runtime environment).
 *
 * NRE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NRE is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
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
