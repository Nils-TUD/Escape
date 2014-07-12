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

class AddressSpace {
	/* rV.n has 10 bits */
	static const size_t ADDR_SPACE_COUNT		= 1024;

public:
	static void init();
	static AddressSpace *alloc();
	static void free(AddressSpace *aspace);

	ulong getNo() const {
		return no;
	}
	ulong getRefCount() const {
		return refCount;
	}

private:
	ulong no;
	ulong refCount;
	struct AddressSpace *next;
	static AddressSpace addrSpaces[ADDR_SPACE_COUNT];
	static AddressSpace *freeList;
	static AddressSpace *usedList;
	static AddressSpace *lastUsed;
};
