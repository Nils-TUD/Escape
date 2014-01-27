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

#include <sys/common.h>
#include <sys/interrupts.h>
#include <sys/mutex.h>

class VM86 {
public:
	struct Regs {
		uint16_t ax;
		uint16_t bx;
		uint16_t cx;
		uint16_t dx;
		uint16_t si;
		uint16_t di;
		uint16_t ds;
		uint16_t es;
	};

	struct AreaPtr {
		uintptr_t offset;	/* the offset to <dst> */
		uintptr_t result;	/* the address in your address-space where to copy the data */
		size_t size;		/* the size of the data */
	};

	struct Memarea {
		/* copy <src> in your process to <dst> in vm86-task before start and the other way around
		 * when finished */
		void *src;
		uintptr_t dst;
		size_t size;
		/* optionally, <ptrCount> pointers to <dst> which are copied to your address-space, that is
		 * if the result produced by the bios (at <dst>) contains pointers to other areas, you can
		 * copy the data in these areas to your address-space as well. */
		AreaPtr *ptr;
		size_t ptrCount;
	};

private:
	struct Info {
		uint16_t interrupt;
		Regs regs;
		Memarea *area;
		void **copies;
	};

public:
	/**
	 * Creates a vm86-task
	 *
	 * @return 0 on success
	 */
	static int create();

	/**
	 * Performs a VM86-interrupt
	 *
	 * @param interrupt the interrupt-number
	 * @param regs the register
	 * @param area the memarea
	 * @return 0 on success
	 */
	static int interrupt(uint16_t interrupt,Regs *regs,const Memarea *area);

	/**
	 * Handles a GPF
	 *
	 * @param stack the interrupt-stack-frame
	 */
	static void handleGPF(VM86IntrptStackFrame *stack);

private:
	static uint16_t popw(VM86IntrptStackFrame *stack);
	static uint32_t popl(VM86IntrptStackFrame *stack);
	static void pushw(VM86IntrptStackFrame *stack,uint16_t word);
	static void pushl(VM86IntrptStackFrame *stack,uint32_t l);
	static void start() A_NORETURN;
	static void stop(VM86IntrptStackFrame *stack);
	static void finish();
	static void copyRegResult(VM86IntrptStackFrame* stack);
	static int storeAreaResult();
	static void copyAreaResult();
	static bool copyInfo(uint16_t interrupt,USER const Regs *regs,USER const Memarea *area);
	static void clearInfo();

	static frameno_t frameNos[];
	static tid_t vm86Tid;
	static volatile tid_t caller;
	static Info info;
	static int vm86Res;
	static Mutex vm86Lock;
};
