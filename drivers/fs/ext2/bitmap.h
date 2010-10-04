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

#ifndef BITMAP_H_
#define BITMAP_H_

#include <esc/common.h>
#include "ext2.h"

/**
 * Allocates a new inode for the given directory-inode. It will be tried to allocate an inode in
 * the same block-group
 *
 * @param e the ext2-data
 * @param dirNode the directory-inode
 * @param isDir whether the inode should be an directory
 * @return the inode-number or 0 if failed
 */
tInodeNo ext2_bm_allocInode(sExt2 *e,sExt2CInode *dirInode,bool isDir);

/**
 * Free's the given inode-number
 *
 * @param e the ext2-data
 * @param ino the inode-number
 * @param isDir whether the inode is an directory
 * @return 0 on success
 */
int ext2_bm_freeInode(sExt2 *e,tInodeNo ino,bool isDir);

/**
 * Allocates a new block for the given inode. It will be tried to allocate a block in the same
 * block-group.
 *
 * @param e the ext2-data
 * @param inode the inode
 * @return the block-number or 0 if failed
 */
tBlockNo ext2_bm_allocBlock(sExt2 *e,sExt2CInode *inode);

/**
 * Free's the given block-number
 *
 * @param e the ext2-data
 * @param blockNo the block-number
 * @return 0 on success
 */
int ext2_bm_freeBlock(sExt2 *e,tBlockNo blockNo);

#endif /* BITMAP_H_ */
