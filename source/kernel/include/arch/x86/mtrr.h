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
#include <spinlock.h>
#include <cpu.h>

class MTRR {
	MTRR() = delete;

	enum {
		CAP_VCNT	= 0xFF,			/* number of variable ranges */
		CAP_FIXED	= 1 << 8,		/* fixed-ranges enabled */
		CAP_WC		= 1 << 10,		/* write combining supported */
		CAP_SMRR	= 1 << 11,		/* system management range registers supported */
	};

	enum {
		DEF_TYPE	= 0xFF,			/* default type */
		DEF_E		= 1 << 11,		/* MTRR enabled */
		DEF_FE		= 1 << 10,		/* fixed-range MTRRs enabled */
	};

	enum {
		BASE_TYPE	= 0xFF,
		MASK_VALID	= 1 << 11,
	};

public:
	enum Type {
		UNCACHEABLE			= 0,
		WRITE_COMBINING		= 1,
		WRITE_THROUGH		= 4,
		WRITE_PROTECTED		= 5,
		WRITE_BACK			= 6,
	};

	static void init();
	static int setRange(uint64_t base,uint64_t length,Type type);
	static void print(OStream &os);

private:
	static SpinLock _lock;
	static uint _addrbits;
	static uint64_t _cap;
};
