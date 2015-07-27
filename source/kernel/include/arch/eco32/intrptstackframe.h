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
#include <ostream.h>

#define REG_COUNT 32

/* the saved registers */
struct IntrptStackFrame {
	static const uint32_t PSW_PUM			= 0x02000000;	/* previous value of PSW_UM */

	bool fromUserSpace() const {
		return psw & PSW_PUM;
	}

	void print(OStream &os) const {
		os.writef("stack-frame @ %p\n",this);
		os.writef("\tint: %d\n",irqNo);
		os.writef("\tpsw: %#08x\n",psw);
		for(int i = 0; i < REG_COUNT; i++)
			os.writef("\tr[%d]=%#08x\n",i,r[i]);
	}

	uint32_t r[REG_COUNT];
	uint32_t psw;
	uint32_t irqNo;
} A_PACKED;

class Thread;
typedef void (*irqhandler_func)(Thread *t,IntrptStackFrame *stack);
