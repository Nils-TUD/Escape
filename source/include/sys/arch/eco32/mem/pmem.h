/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#ifndef ECO32_PMEM_H_
#define ECO32_PMEM_H_

#include <esc/common.h>

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |          kernel code+data         |     |
 *             |                                   |
 *             +-----------------------------------+  unmanaged
 *             |         multiboot-modules         |
 *             +-----------------------------------+     |
 *             |              MM-stack             |     |
 *             +-----------------------------------+   -----
 *             |             MM-bitmap             |     |
 *             +-----------------------------------+     |
 *             |                                   |
 *             |                                   |  bitmap managed (2 MiB)
 *             |                ...                |
 *             |                                   |     |
 *             |                                   |     |
 *             +-----------------------------------+   -----
 *             |                                   |     |
 *             |                                   |
 *             |                ...                |  stack managed (remaining)
 *             |                                   |
 *             |                                   |     |
 *             +-----------------------------------+   -----
 *             |                ...                |
 * 0xE0000000: +-----------------------------------+
 *             |                                   |
 *             |                ROM                |
 *             |                                   |
 * 0xF0000000: +-----------------------------------+
 *             |                                   |
 *             |         memory mapped I/O         |
 *             |                                   |
 * 0xFFFFFFFF: +-----------------------------------+
 */

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12

#define BITMAP_PAGE_COUNT		((2 * M) / PAGE_SIZE)
/* the end is not important here, since the mm-stack lies in physical memory and we will simply
 * use as much as we need, without the possibility to overwrite anything */
#define PMEM_END				0xE0000000

typedef ulong tBitmap;

#endif /* ECO32_PMEM_H_ */
