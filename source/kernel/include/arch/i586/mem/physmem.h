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

/**
 * Physical memory layout:
 * 0x00000000: +-----------------------------------+
 *             |                                   |
 *             |             BIOS stuff            |
 *             |                                   |
 * 0x00100000: +-----------------------------------+
 *             |                                   |
 *             |          kernel code+data         |
 *             |                                   |
 *             +-----------------------------------+
 *             |                ...                |
 * 0xFFFFFFFF: +-----------------------------------+
 */

/* the physical start-address of the kernel-area */
#define KERNEL_AREA_P_ADDR		0x0
/* the physical start-address of the kernel itself */
#define KERNEL_P_ADDR			(1 * 1024 * 1024)

#define PAGE_BITS				12
#define PAGE_SIZE				(1 << PAGE_BITS)
#define PHYS_BITS				32

#define BITMAP_PAGE_COUNT		((2 * 1024 * 1024) / PAGE_SIZE)
