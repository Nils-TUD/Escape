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

class Thread;

class Interrupts : public InterruptsBase {
	friend class InterruptsBase;

	Interrupts() = delete;

	/* interrupt- and exception-numbers */
	enum {
		IRQ_TTY0_XMTR		 = 0,
		IRQ_TTY0_RCVR		 = 1,
		IRQ_TTY1_XMTR		 = 2,
		IRQ_TTY1_RCVR		 = 3,
		IRQ_KEYBOARD		 = 4,
		IRQ_DISK			 = 8,
		IRQ_TIMER			 = 14,
		EXC_BUS_TIMEOUT		 = 16,
		EXC_ILL_INSTRCT		 = 17,
		EXC_PRV_INSTRCT		 = 18,
		EXC_DIVIDE			 = 19,
		EXC_TRAP			 = 20,
		EXC_TLB_MISS		 = 21,
		EXC_TLB_WRITE		 = 22,
		EXC_TLB_INVALID		 = 23,
		EXC_ILL_ADDRESS		 = 24,
		EXC_PRV_ADDRESS		 = 25,
	};

	static const uint32_t PSW_PUM			= 0x02000000;	/* previous value of PSW_UM */

	static const uintptr_t KEYBOARD_BASE	= 0xF0200000;
	static const ulong KEYBOARD_CTRL		= 0;
	static const ulong KEYBOARD_IEN			= 0x02;

	static const uintptr_t DISK_BASE		= 0xF0400000;
	static const ulong DISK_CTRL			= 0;
	static const ulong DISK_IEN				= 0x02;

	typedef void (*handler_func)(IntrptStackFrame *stack);
	struct Interrupt {
		handler_func handler;
		const char *name;
		int signal;
	};

	static void defHandler(IntrptStackFrame *stack);
	static void debug(IntrptStackFrame *stack);
	static void exTrap(IntrptStackFrame *stack);
	static void exPageFault(IntrptStackFrame *stack);
	static void irqTimer(IntrptStackFrame *stack);
	static void irqKB(IntrptStackFrame *stack);
	static void irqDisk(IntrptStackFrame *stack);

	static size_t irqCount;
	static uintptr_t pfaddr;
	static const Interrupt intrptList[];
};

inline size_t InterruptsBase::getCount() {
	return Interrupts::irqCount;
}
