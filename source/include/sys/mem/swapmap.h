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

#ifndef SWAPMAP_H_
#define SWAPMAP_H_

#include <sys/common.h>
#include <esc/sllist.h>

#define INVALID_BLOCK		0xFFFFFFFF

/**
 * Inits the swap-map
 *
 * @param swapSize the size of the swap-device in bytes
 */
void swmap_init(size_t swapSize);

/**
 * Allocates 1 block on the swap-device
 *
 * @return the starting block on the swap-device or INVALID_BLOCK if no free space is left
 */
ulong swmap_alloc(void);

/**
 * @param block the block-number
 * @return true if the given block is used
 */
bool swmap_isUsed(ulong block);

/**
 * Determines the free space in the swapmap
 *
 * @return the free space in bytes
 */
size_t swmap_freeSpace(void);

/**
 * Free's the given block
 *
 * @param block the block to free
 */
void swmap_free(ulong block);

/**
 * Prints the swap-map
 */
void swmap_print(void);

#endif /* SWAPMAP_H_ */
