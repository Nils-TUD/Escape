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

#define PANIC_ON_PAGEFAULT	1
#define IRQ_COUNT			0x3A

class Thread;
class LAPIC;

class Interrupts : public InterruptsBase {
	friend class InterruptsBase;
	friend class LAPIC;

	Interrupts() = delete;

public:
	/* we remap the IRQs to 0x20 and 0x28 so that 0..0x1f is free for the protected-mode-exceptions */
	static const int IRQ_NUM				= 16;
	static const int IRQ_MASTER_BASE 		= 0x20;
	static const int IRQ_SLAVE_BASE			= 0x28;

	/* IRQs */
	enum {
		IRQ_TIMER			= IRQ_MASTER_BASE + 0,
		IRQ_KEYBOARD		= IRQ_MASTER_BASE + 1,
		/* IRQ_MASTER_BASE + 2 = cascade */
		IRQ_COM2			= IRQ_MASTER_BASE + 3,
		IRQ_COM1			= IRQ_MASTER_BASE + 4,
		IRQ_FLOPPY			= IRQ_MASTER_BASE + 6,
		/* IRQ_MASTER_BASE + 7 = ? */
		IRQ_CMOS_RTC		= IRQ_MASTER_BASE + 8,
		IRQ_LAPIC			= IRQ_MASTER_BASE + 9,
		IRQ_MOUSE			= IRQ_MASTER_BASE + 12,
		IRQ_ATA1			= IRQ_MASTER_BASE + 14,
		IRQ_ATA2			= IRQ_MASTER_BASE + 15,
		/* irq-number for a syscall */
		IRQ_SYSCALL			= 0x30
	};

	/* Exceptions */
	enum {
		/*
		 * Occurs during a DIV or an IDIV instruction when the
		 * divisor is zero or a quotient overflow occurs.
		 */
		EX_DIVIDE_BY_ZERO	= 0x0,

		/*
		 * Occurs for any of a number of conditions:
		 * - Instruction address breakpoint fault
		 * - Data address breakpoint trap
		 * - General detect fault
		 * - Single-step trap
		 * - Task-switch breakpoint trap
		 */
		EX_SINGLE_STEP		= 0x1,

		/*
		 * Occurs because of a nonmaskable hardware interrupt.
		 */
		EX_NONMASKABLE		= 0x2,

		/*
		 * Occurs when the processor encounters an INT 3 instruction.
		 */
		EX_BREAKPOINT		= 0x3,

		/*
		 * Occurs when the processor encounters an INTO instruction
		 * and the OF (overflow) flag is set.
		 */
		EX_OVERFLOW			= 0x4,

		/*
		 * Occurs when the processor, while executing a BOUND
		 * instruction, finds that the operand exceeds the specified
		 * limit.
		 */
		EX_BOUNDS_CHECK		= 0x5,

		/*
		 * Occurs when an invalid opcode is detected.
		 */
		EX_INVALID_OPCODE	= 0x6,

		/*
		 * Coprocessor not available:
		 * Occurs for one of two conditions:
		 * - The processor encounters an ESC (escape) instruction
		 *   and the EM (emulate) bit of CR0 (control register zero)
		 *   is set.
		 * - The processor encounters either the WAIT instruction or
		 *   an ESC instruction and both the MP (monitor
		 *   coprocessor) and TS (task switched) bits of CR0 are set.
		 */
		EX_CO_PROC_NA		= 0x7,

		/*
		 * Occurs when the processor detects an exception while
		 * trying to invoke the handler for a prior exception.
		 */
		EX_DOUBLE_FAULT		= 0x8,

		/*
		 * Occurs when a page or segment violation is detected while
		 * transferring the middle portion of a coprocessor operand
		 * to the NPX.
		 */
		EX_CO_PROC_SEG_OVR	= 0x9,

		/*
		 * Occurs if, during a task switch, the new TSS is invalid.
		 */
		EX_INVALID_TSS		= 0xA,

		/*
		 * Occurs when the processor detects that the present bit of a descriptor is zero.
		 */
		EX_SEG_NOT_PRESENT	= 0xB,

		/*
		 * Occurs for one of two conditions:
		 * - As a result of a limit violation in any operation that
		 *   refers to SS (stack segment register)
		 * - When attempting to load SS with a descriptor that is
		 *   marked as not-present but is otherwise valid
		 */
		EX_STACK			= 0xC,

		/*
		 * General protection violation:
		 * Each protection violation that does not cause another
		 * exception causes a general protection exception.
		 * - Exceeding segment limit when using CS, DS, ES, FS, or GS
		 * - Exceeding segment limit when referencing a descriptor table
		 * - Transferring control to a segment that is not executable
		 * - Writing to a read-only data segment or a code segment
		 * - Reading from an execute-only segment
		 * - Loading SS with a read-only descriptor
		 * - Loading SS, DS, ES, FS, or GS with a descriptor of a system
		 *   segment
		 * - Loading DS, ES, FS, or GS with the descriptor of an
		 *   executable segment that is also not readable
		 * - Loading SS with the descriptor of an executable segment
		 * - Accessing memory through DS, ES, FS, or GS when the segment
		 *   register contains a NULL selector
		 * - Switching to a busy task
		 * - Violating privilege rules
		 * - Loading CR0 with PG=1 and PE=0
		 * - Interrupt or exception through trap or interrupt gate from
		 *   V86 mode to a privilege level other than 0
		 */
		EX_GEN_PROT_FAULT	= 0xD,

		/*
		 * Occurs when paging is enabled (PG=1) and the processor
		 * detects one of the following conditions while translating
		 * a linear address to a physical address:
		 * - The page-directory or page-table entry needed for the
		 *   address translation has 0 in its present bit.
		 * - The current procedure does not have sufficient privilege
		 *   to access the indicated page.
		 */
		EX_PAGE_FAULT		= 0xE,

		/* 0xF is reserved */

		/*
		 * Occurs when the processor detects a signal from the
		 * coprocessor on the ERROR# input pin.
		 */
		EX_CO_PROC_ERROR	= 0x10,
	};

private:
	static void syscall(IntrptStackFrame *stack) asm("syscall_handler");

	/**
	 * The exception and interrupt-handlers
	 */
	static void debug(Thread *t,IntrptStackFrame *stack);
	static void exFatal(Thread *t,IntrptStackFrame *stack);
	static void exSStep(Thread *t,IntrptStackFrame *stack);
	static void exGPF(Thread *t,IntrptStackFrame *stack);
	static void exCoProcNA(Thread *t,IntrptStackFrame *stack);
	static void exPF(Thread *t,IntrptStackFrame *stack);
	static void irqTimer(Thread *t,IntrptStackFrame *stack);
	static void irqDefault(Thread *t,IntrptStackFrame *stack);
	static void ipiWork(Thread *t,IntrptStackFrame *stack);
	static void ipiTerm(Thread *t,IntrptStackFrame *stack);
	static void ipiCallback(Thread *t,IntrptStackFrame *stack);

	static void eoi(int irq);
	static void printPFInfo(OStream &os,Thread *t,IntrptStackFrame *stack,uintptr_t pfaddr);

	static uintptr_t *pfAddrs;
	/* stuff to count exceptions */
	static size_t exCount;
	static uint32_t lastEx;
};

inline uint8_t InterruptsBase::getVectorFor(uint8_t irq) {
	return irq + 0x20;
}
