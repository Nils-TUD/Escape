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

#ifndef INTRPT_H_
#define INTRPT_H_

#include <sys/common.h>

/* we remap the IRQs to 0x20 and 0x28 so that 0..0x1f is free for the protected-mode-exceptions */
#define IRQ_NUM				16
#define IRQ_MASTER_BASE 	0x20
#define	IRQ_SLAVE_BASE		0x28


/* IRQs */
#define IRQ_TIMER			IRQ_MASTER_BASE + 0
#define IRQ_KEYBOARD		IRQ_MASTER_BASE + 1
/* IRQ_MASTER_BASE + 2 = cascade */
#define IRQ_COM2			IRQ_MASTER_BASE + 3
#define IRQ_COM1			IRQ_MASTER_BASE + 4
#define IRQ_FLOPPY			IRQ_MASTER_BASE + 6
/* IRQ_MASTER_BASE + 7 = ? */
#define IRQ_CMOS_RTC		IRQ_MASTER_BASE + 8
#define IRQ_MOUSE			IRQ_MASTER_BASE + 12
#define IRQ_ATA1			IRQ_MASTER_BASE + 14
#define IRQ_ATA2			IRQ_MASTER_BASE + 15

/* irq-number for a syscall */
#define IRQ_SYSCALL			0x30


/* Exceptions */

/*
 * Occurs during a DIV or an IDIV instruction when the
 * divisor is zero or a quotient overflow occurs.
 */
#define EX_DIVIDE_BY_ZERO	0x0

/*
 * Occurs for any of a number of conditions:
 * - Instruction address breakpoint fault
 * - Data address breakpoint trap
 * - General detect fault
 * - Single-step trap
 * - Task-switch breakpoint trap
 */
#define EX_SINGLE_STEP		0x1

/*
 * Occurs because of a nonmaskable hardware interrupt.
 */
#define EX_NONMASKABLE		0x2

/*
 * Occurs when the processor encounters an INT 3 instruction.
 */
#define EX_BREAKPOINT		0x3

/*
 * Occurs when the processor encounters an INTO instruction
 * and the OF (overflow) flag is set.
 */
#define EX_OVERFLOW			0x4

/*
 * Occurs when the processor, while executing a BOUND
 * instruction, finds that the operand exceeds the specified
 * limit.
 */
#define EX_BOUNDS_CHECK		0x5

/*
 * Occurs when an invalid opcode is detected.
 */
#define EX_INVALID_OPCODE	0x6

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
#define EX_CO_PROC_NA		0x7

/*
 * Occurs when the processor detects an exception while
 * trying to invoke the handler for a prior exception.
 */
#define EX_DOUBLE_FAULT		0x8

/*
 * Occurs when a page or segment violation is detected while
 * transferring the middle portion of a coprocessor operand
 * to the NPX.
 */
#define EX_CO_PROC_SEG_OVR	0x9

/*
 * Occurs if, during a task switch, the new TSS is invalid.
 */
#define EX_INVALID_TSS		0xA

/*
 * Occurs when the processor detects that the present bit of a descriptor is zero.
 */
#define	EX_SEG_NOT_PRESENT	0xB

/*
 * Occurs for one of two conditions:
 * - As a result of a limit violation in any operation that
 *   refers to SS (stack segment register)
 * - When attempting to load SS with a descriptor that is
 *   marked as not-present but is otherwise valid
 */
#define EX_STACK			0xC

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
#define EX_GEN_PROT_FAULT	0xD

/*
 * Occurs when paging is enabled (PG=1) and the processor
 * detects one of the following conditions while translating
 * a linear address to a physical address:
 * - The page-directory or page-table entry needed for the
 *   address translation has 0 in its present bit.
 * - The current procedure does not have sufficient privilege
 *   to access the indicated page.
 */
#define EX_PAGE_FAULT		0xE

/* 0xF is reserved */

/*
 * Occurs when the processor detects a signal from the
 * coprocessor on the ERROR# input pin.
 */
#define EX_CO_PROC_ERROR	0x10


/* the stack frame for the interrupt-handler */
typedef struct {
	/* stack-pointer before calling isr-handler */
	u32 esp;
	/* segment-registers */
	u32 es;
	u32 ds;
	u32 fs;
	u32 gs;
	/* general purpose registers */
	u32 edi;
	u32 esi;
	u32 ebp;
	u32 : 32; /* esp from pusha */
	u32 ebx;
	u32 edx;
	u32 ecx;
	u32 eax;
	/* interrupt-number */
	u32 intrptNo;
	/* error-code (for exceptions); default = 0 */
	u32 errorCode;
	/* pushed by the CPU */
	u32 eip;
	u32 cs;
	u32 eflags;
	/* if we come from user-mode this fields will be present and will be restored with iret */
	u32 uesp;
	u32 uss;
	/* available when interrupting an vm86-task */
	u16 vm86es;
	u16 : 16;
	u16 vm86ds;
	u16 : 16;
	u16 vm86fs;
	u16 : 16;
	u16 vm86gs;
	u16 : 16;
} A_PACKED sIntrptStackFrame;

/**
 * Enables / disables the interrupts
 *
 * @param enabled the new value
 * @return the old value
 */
extern bool intrpt_setEnabled(bool enabled);

/**
 * Initializes the interrupts
 */
void intrpt_init(void);

/**
 * @return the total number of interrupts so far
 */
u32 intrpt_getCount(void);

/**
 * @return the current interrupt-stack (may be NULL)
 */
sIntrptStackFrame *intrpt_getCurStack(void);

/**
 * Handles an interrupt
 *
 * @param number the interrupt-number
 */
void intrpt_handler(sIntrptStackFrame *stack);

#if DEBUGGING

/**
 * Prints the given interrupt-stack
 *
 * @param stack the interrupt-stack
 */
void intrpt_dbg_printStackFrame(sIntrptStackFrame *stack);

#endif

#endif /*INTRPT_H_*/
