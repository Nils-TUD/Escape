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

#include <sys/common.h>

class PhysMem;

class PhysMemAreas {
	friend class PhysMem;

	PhysMemAreas() = delete;

	static const size_t MAX_PHYSMEM_AREAS			= 64;

public:
	struct MemArea {
		uintptr_t addr;
		size_t size;
		MemArea *next;
	};

	/**
	 * Allocates <frames> frames from the physical memory. That is, it removes it from the area-list.
	 *
	 * @param frames the number of frames
	 * @return the first frame-number
	 */
	static frameno_t alloc(size_t frames);

	/**
	 * @return the list of free physical memory
	 */
	static const MemArea *get() {
		return list;
	}

	/**
	 * @return the total amount of memory (in bytes)
	 */
	static size_t getTotal();

	/**
	 * @return the currently available amount of memory (in bytes)
	 */
	static size_t getAvailable();

	/**
	 * Prints the list
	 */
	static void print();

	/* TODO private! */
	/**
	 * Adds <addr>..<end> to the list of free memory
	 *
	 * @param addr the start address
	 * @param end the end address
	 */
	static void add(uintptr_t addr,uintptr_t end);

	/**
	 * Removes <addr>..<end> from all areas in the list
	 *
	 * @param addr the start address
	 * @param end the end address
	 */
	static void rem(uintptr_t addr,uintptr_t end);

private:
	/**
	 * Architecture-dependent: collects all available memory
	 */
	static void initArch();
	static MemArea *allocArea();

	static MemArea areas[];
	static size_t count;
	static MemArea *list;
	static MemArea *listend;
};
