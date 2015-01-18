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

#include <arch/x86/desc.h>
#include <common.h>

class IDT {
	IDT() = delete;

#if defined(__x86_64__)
	typedef Desc64 Entry;
#else
	typedef Desc Entry;
#endif

	/* isr prototype */
	typedef void (*isr_func)();

	/* reserved by intel */
	enum {
		INTEL_RES1			= 2,
		INTEL_RES2			= 15
	};

	static const size_t IDT_COUNT		= 256;

public:
	/**
	 * Inits the IDT for the current processor
	 */
	static void init();

private:
	static void load(DescTable *tbl) {
		asm volatile ("lidt %0" : : "m"(*tbl));
	}
	static void set(size_t number,isr_func handler,uint8_t dpl);

	/* the interrupt descriptor table */
	static Entry idt[IDT_COUNT];
};
