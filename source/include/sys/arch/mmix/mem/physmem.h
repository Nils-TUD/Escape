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
 * 0x0000000000000000: +-----------------------------------+
 *                     |                                   |
 *                     |          kernel code+data         |
 *                     |                                   |
 *                     +-----------------------------------+
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

#define PAGE_BITS				13
#define PAGE_SIZE				(1 << PAGE_BITS)
#define PHYS_BITS				64

#define BITMAP_PAGE_COUNT		((6 * 1024 * 1024) / PAGE_SIZE)
