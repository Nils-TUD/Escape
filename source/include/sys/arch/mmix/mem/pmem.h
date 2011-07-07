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

#ifndef MMIX_PMEM_H_
#define MMIX_PMEM_H_

#include <esc/common.h>

/**
 * Physical memory layout:
 * 0x0000000000000000: +-----------------------------------+   -----
 *                     |                                   |     |
 *                     |          kernel code+data         |     |
 *                     |                                   |
 *                     +-----------------------------------+  unmanaged
 *                     |         multiboot-modules         |
 *                     +-----------------------------------+     |
 *                     |              MM-stack             |     |
 *                     +-----------------------------------+   -----
 *                     |             MM-bitmap             |     |
 *                     +-----------------------------------+     |
 *                     |                                   |
 *                     |                                   |  bitmap managed (2 MiB)
 *                     |                ...                |
 *                     |                                   |     |
 *                     |                                   |     |
 *                     +-----------------------------------+   -----
 *                     |                                   |     |
 *                     |                                   |
 *                     |                ...                |  stack managed (remaining)
 *                     |                                   |
 *                     |                                   |     |
 *                     +-----------------------------------+   -----
 *                     |                ...                |
 * 0x0000FFFF00000000: +-----------------------------------+
 *                     |                                   |
 *                     |                ROM                |
 *                     |                                   |
 * 0x0001000000000000: +-----------------------------------+
 *                     |                                   |
 *                     |                                   |
 *                     |         memory mapped I/O         |
 *                     |                                   |
 *                     |                                   |
 * 0xFFFFFFFFFFFFFFFF: +-----------------------------------+
 */

#define PAGE_SIZE				((size_t)(8 * K))
#define PAGE_SIZE_SHIFT			13

#define BITMAP_PAGE_COUNT		((6 * M) / PAGE_SIZE)
/* the end is not important here, since the mm-stack lies in physical memory and we will simply
 * use as much as we need, without the possibility to overwrite anything */
#define PMEM_END				0x8000FFFF00000000

typedef ulong tBitmap;

#endif /* MMIX_PMEM_H_ */
