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

#ifndef BLOCKCACHE_H_
#define BLOCKCACHE_H_

#include <esc/common.h>
#include "ext2.h"

/**
 * Inits the block-cache
 *
 * @param e the ext2-handle
 */
void ext2_bcache_init(sExt2 *e);

/**
 * Writes all dirty blocks to disk
 *
 * @param e the ext2-handle
 */
void ext2_bcache_flush(sExt2 *e);

/**
 * Creates a new block-cache-entry for given block-number. Does not read the contents from disk!
 *
 * @param e the ext2-handle
 * @param blockNo the block-number
 * @return the block or NULL
 */
sExt2CBlock *ext2_bcache_create(sExt2 *e,u32 blockNo);

/**
 * Requests the block with given number
 *
 * @param e the ext2-handle
 * @param blockNo the block to fetch from disk or from the cache
 * @return the block or NULL
 */
sExt2CBlock *ext2_bcache_request(sExt2 *e,u32 blockNo);

/**
 * Prints the used and free blocks
 *
 * @param e the ext2-handle
 */
void ext2_bcache_print(sExt2 *e);

/**
 * Prints block-cache statistics
 */
void ext2_bcache_printStats(void);

#endif /* BLOCKCACHE_H_ */
