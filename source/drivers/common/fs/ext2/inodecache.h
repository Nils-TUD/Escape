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

#ifndef INODECACHE_H_
#define INODECACHE_H_

#include <esc/common.h>
#include "ext2.h"

#define IMODE_READ		0x1
#define IMODE_WRITE		0x2

/**
 * Inits the inode-cache
 *
 * @param e the ext2-handle
 */
void ext2_icache_init(sExt2 *e);

/**
 * Writes all dirty inodes to disk
 *
 * @param e the ext2-handle
 */
void ext2_icache_flush(sExt2 *e);

/**
 * Marks the given inode dirty
 *
 * @param inode the inode
 */
void ext2_icache_markDirty(sExt2CInode *inode);

/**
 * Requests the inode with given number. That means if it is in the cache you'll simply get it.
 * Otherwise it is fetched from disk and put into the cache. The references of the cache-inode
 * will be increased.
 *
 * @param e the ext2-handle
 * @param no the inode-number
 * @param mode the mode: IMODE_*
 * @return the cached node or NULL
 */
sExt2CInode *ext2_icache_request(sExt2 *e,tInodeNo no,uint mode);

/**
 * Releases the given inode. That means the references will be decreased and the inode will be
 * removed from cache if there are no more references.
 *
 * @param inode the inode
 */
void ext2_icache_release(const sExt2CInode *inode);

/**
 * Prints inode-cache statistics
 */
void ext2_icache_printStats(void);

#endif /* INODECACHE_H_ */
