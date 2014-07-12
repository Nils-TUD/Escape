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

#define STACK_REG_COUNT			1

/* the thread-state which will be saved for context-switching */
struct ThreadRegs {
	void print(OStream &os) const {
		os.writef("State:\n",this);
		os.writef("\t$16 = %#08x\n",r16);
		os.writef("\t$17 = %#08x\n",r17);
		os.writef("\t$18 = %#08x\n",r18);
		os.writef("\t$19 = %#08x\n",r19);
		os.writef("\t$20 = %#08x\n",r20);
		os.writef("\t$21 = %#08x\n",r21);
		os.writef("\t$22 = %#08x\n",r22);
		os.writef("\t$23 = %#08x\n",r23);
		os.writef("\t$29 = %#08x\n",r29);
		os.writef("\t$30 = %#08x\n",r30);
		os.writef("\t$31 = %#08x\n",r31);
	}

	uint32_t r16;
	uint32_t r17;
	uint32_t r18;
	uint32_t r19;
	uint32_t r20;
	uint32_t r21;
	uint32_t r22;
	uint32_t r23;
	uint32_t r25;
	uint32_t r29;
	uint32_t r30;
	uint32_t r31;
};
