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

typedef struct sPhysMemArea {
	uintptr_t addr;
	size_t size;
	struct sPhysMemArea *next;
} sPhysMemArea;

/**
 * Architecture-dependent: collects all available memory
 */
void pmemareas_initArch(void);

/**
 * Allocates <frames> frames from the physical memory. That is, it removes it from the area-list.
 *
 * @param frames the number of frames
 * @return the first frame-number
 */
frameno_t pmemareas_alloc(size_t frames);

/**
 * @return the list of free physical memory
 */
sPhysMemArea *pmemareas_get(void);

/**
 * @return the total amount of memory (in bytes)
 */
size_t pmemareas_getTotal(void);

/**
 * @return the currently available amount of memory (in bytes)
 */
size_t pmemareas_getAvailable(void);

/**
 * Adds <addr>..<end> to the list of free memory
 *
 * @param addr the start address
 * @param end the end address
 */
void pmemareas_add(uintptr_t addr,uintptr_t end);

/**
 * Removes <addr>..<end> from all areas in the list
 *
 * @param addr the start address
 * @param end the end address
 */
void pmemareas_rem(uintptr_t addr,uintptr_t end);

/**
 * Prints the list
 */
void pmemareas_print(void);
