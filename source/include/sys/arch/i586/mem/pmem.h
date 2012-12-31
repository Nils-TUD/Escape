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

#include <esc/common.h>
#include <sys/arch/i586/mem/paging.h>

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+   -----
 *             |                                   |     |
 *             |                VM86               |     |
 *             |                                   |     |
 * 0x00100000: +-----------------------------------+     |
 *             |                                   |
 *             |          kernel code+data         |  unmanaged
 *             |                                   |
 *             +-----------------------------------+     |
 *             |         multiboot-modules         |     |
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
 * 0xFFFFFFFF: +-----------------------------------+   -----
 */

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * M)

#define PAGE_SIZE				(4 * K)
#define PAGE_SIZE_SHIFT			12

#define BITMAP_START			bitmapStart
#define BITMAP_PAGE_COUNT		((2 * M) / PAGE_SIZE)
#define PMEM_END				(BOOTSTRAP_AREA + BOOTSTRAP_AREA_SIZE)

typedef ulong tBitmap;
extern uintptr_t bitmapStart;
