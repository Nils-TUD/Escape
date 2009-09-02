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

#ifndef SUPERBLOCK_H_
#define SUPERBLOCK_H_

#include <esc/common.h>
#include "ext2.h"

/**
 * Inits the super-block; reads it from disk and reads the block-group-descriptor-table
 *
 * @param e the ext2-data
 * @return true if successfull
 */
bool ext2_initSuperBlock(sExt2 *e);

/**
 * Writes all dirty objects of the filesystem to disk
 *
 * @param e the ext2-data
 */
void ext2_sync(sExt2 *e);

/**
 * Determines the block of the given inode
 *
 * @param e the ext2-data
 * @param inodeNo the inode-number
 * @return the block-number
 */
u32 ext2_getBlockOfInode(sExt2 *e,tInodeNo inodeNo);

/**
 * Determines the block-group of the given block
 *
 * @param e the ext2-data
 * @param block the block-number
 * @return the block-group-number
 */
u32 ext2_getGroupOfBlock(sExt2 *e,u32 block);

/**
 * Allocates a new block for the given inode. It will be tried to allocate a block in the same
 * block-group.
 *
 * @param e the ext2-data
 * @param inode the inode
 * @return the block-number or 0 if failed
 */
u32 ext2_allocBlock(sExt2 *e,sCachedInode *inode);

/**
 * Free's the given block-number
 *
 * @param e the ext2-data
 * @param blockNo the block-number
 * @return 0 on success
 */
s32 ext2_freeBlock(sExt2 *e,u32 blockNo);

/**
 * Updates the super-block, if it is dirty
 *
 * @param e the ext2-data
 */
void ext2_updateSuperBlock(sExt2 *e);

/**
 * Updates the block-group-descriptor-table, if it is dirty
 *
 * @param e the ext2-data
 */
void ext2_updateBlockGroups(sExt2 *e);

/**
 * @param e the ext2-data
 * @return the number of block-groups
 */
u32 ext2_getBlockGroupCount(sExt2 *e);

#endif /* SUPERBLOCK_H_ */
