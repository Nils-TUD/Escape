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

#include <esc/common.h>

class IDT {
	IDT() = delete;

	/* represents an IDT-entry */
	struct Entry {
		/* The address[0..15] of the ISR */
		uint16_t offsetLow;
		/* Code selector that the ISR will use */
		uint16_t selector;
		/* these bits are fix: 0.1110.0000.0000b */
		uint16_t fix		: 13,
		/* the privilege level, 00 = ring0, 01 = ring1, 10 = ring2, 11 = ring3 */
		dpl					: 2,
		/* If Present is not set to 1, an exception will occur */
		present				: 1;
		/* The address[16..31] of the ISR */
		uint16_t offsetHigh;
		/* for 64-bit, we have 32-bit more offset */
#if defined(__x86_64__)
		uint32_t offsetUpper;
		uint32_t : 32;
#endif
	} A_PACKED;

	struct Entry64 : public Entry {
	} A_PACKED;

	/* represents an IDT-pointer */
	struct Pointer {
		uint16_t size;
		ulong address;
	} A_PACKED;

	/* isr prototype */
	typedef void (*isr_func)();

	/* the privilege level */
	enum {
		DPL_KERNEL			= 0,
		DPL_USER			= 3
	};
	/* reserved by intel */
	enum {
		INTEL_RES1			= 2,
		INTEL_RES2			= 15
	};
	/* the code-selector */
	enum {
		CODE_SEL			= 0x8
	};

	static const size_t IDT_COUNT		= 256;

public:
	/**
	 * Inits the IDT for the current processor
	 */
	static void init();

private:
	static void load(Pointer *ptr) {
		asm volatile ("lidt %0" : : "m"(*ptr));
	}
	static void set(size_t number,isr_func handler,uint8_t dpl);

	/* the interrupt descriptor table */
	static Entry idt[IDT_COUNT];
};
