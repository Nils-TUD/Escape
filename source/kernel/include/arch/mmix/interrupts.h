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

#pragma once

#include <common.h>

#define IRQ_COUNT			64

class Thread;

class Interrupts : public InterruptsBase {
	friend class InterruptsBase;

	Interrupts() = delete;

	enum {
		TRAP_POWER_FAILURE		 = 0,	/* power failure */
		TRAP_MEMORY_PARITY		 = 1,	/* memory parity error */
		TRAP_NONEX_MEMORY		 = 2,	/* non-existent memory */
		TRAP_REBOOT				 = 3,	/* reboot */
		TRAP_INTERVAL			 = 7,	/* interval-counter reached zero */

		TRAP_PRIVILEGED_PC		 = 32,	/* p: comes from a privileged (negative) virt addr */
		TRAP_SEC_VIOLATION		 = 33,	/* s: violates security */
		TRAP_BREAKS_RULES		 = 34,	/* b: breaks the rules of MMIX */
		TRAP_PRIV_INSTR			 = 35,	/* k: is privileged, for use by the "kernel" only */
		TRAP_PRIV_ACCESS		 = 36,	/* n: refers to a negative virtual address */
		TRAP_PROT_EXEC			 = 37,	/* x: appears in a page without execute permission */
		TRAP_PROT_WRITE			 = 38,	/* w: tries to store to a page without write perm */
		TRAP_PROT_READ			 = 39,	/* r: tries to load from a page without read perm */

		TRAP_DISK				 = 51,	/* disk interrupt */
		TRAP_TIMER				 = 52,	/* timer interrupt */
		TRAP_TTY0_XMTR			 = 53,	/* terminal 0 transmitter interrupt */
		TRAP_TTY0_RCVR			 = 54,	/* terminal 0 transmitter interrupt */
		TRAP_TTY1_XMTR			 = 55,	/* terminal 1 transmitter interrupt */
		TRAP_TTY1_RCVR			 = 56,	/* terminal 1 transmitter interrupt */
		TRAP_TTY2_XMTR			 = 57,	/* terminal 2 transmitter interrupt */
		TRAP_TTY2_RCVR			 = 58,	/* terminal 2 transmitter interrupt */
		TRAP_TTY3_XMTR			 = 59,	/* terminal 3 transmitter interrupt */
		TRAP_TTY3_RCVR			 = 60,	/* terminal 3 transmitter interrupt */
		TRAP_KEYBOARD			 = 61,	/* keyboard interrupt */
	};

	static const uintptr_t KEYBOARD_BASE	= 0x8006000000000000;
	static const ulong KEYBOARD_CTRL		= 0;
	static const ulong KEYBOARD_DATA		= 1;
	static const ulong KEYBOARD_IEN			= 0x02;

	static const uintptr_t DISK_BASE		= 0x8003000000000000;
	static const ulong DISK_CTRL			= 0;
	static const ulong DISK_IEN				= 0x02;

	static const uintptr_t TERM_BASE		= 0x8002000000000000;
	static const ulong TERM_RCVR_CTRL		= 0;
	static const ulong TERM_XMTR_CTRL		= 2;
	static const ulong TERM_RCVR_IEN		= 0x02;
	static const ulong TERM_XMTR_IEN		= 0x02;

	static bool kbInstalled;

	static void forcedTrap(IntrptStackFrame *stack) asm("intrpt_forcedTrap");
	static bool dynTrap(IntrptStackFrame *stack,int irqNo) asm("intrpt_dynTrap");

	static void defHandler(IntrptStackFrame *stack,int irqNo);
	static void exNonExMem(IntrptStackFrame *stack,int irqNo);
	static void exProtFault(IntrptStackFrame *stack,int irqNo);
	static void irqKB(IntrptStackFrame *stack,int irqNo);
	static void irqTimer(IntrptStackFrame *stack,int irqNo);
	static void irqDisk(IntrptStackFrame *stack,int irqNo);
	static void irqTermRcvr(IntrptStackFrame *stack,int irqNo);
	static void irqTermXmtr(IntrptStackFrame *stack,int irqNo);

	static void termUser(int irqNo,int res);
public:
	static void printStackFrame(OStream &os,const IntrptStackFrame *stack);
};
