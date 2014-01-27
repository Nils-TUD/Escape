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

#include <esc/common.h>
#include "ext2.h"

/**
 * Creates a new inode for the given directory
 *
 * @param e the ext2-handle
 * @param u the user
 * @param dirNode the directory-inode
 * @param ino will be set to the inode on success
 * @param isDir whether it should be an directory
 * @return 0 on success
 */
int ext2_inode_create(sExt2 *e,sFSUser *u,sExt2CInode *dirNode,sExt2CInode **ino,bool isDir);

/**
 * Sets the mode of the given inode to <mode>
 *
 * @param e the ext2-handle
 * @param u the user
 * @param inodeNo the inode-number
 * @param mode the new mode
 * @return 0 on success
 */
int ext2_inode_chmod(sExt2 *e,sFSUser *u,inode_t inodeNo,mode_t mode);

/**
 * Sets the user and group of the given inode to <uid> and <gid>
 *
 * @param e the ext2-handle
 * @param u the user
 * @param inodeNo the inode-number
 * @param uid the new user-id (-1 = don't change)
 * @param gid the new group-id (-1 = don't change)
 * @return 0 on success
 */
int ext2_inode_chown(sExt2 *e,sFSUser *u,inode_t inodeNo,uid_t uid,gid_t gid);

/**
 * Destroys the given inode. That means the inode will be marked as free in the bitmap,
 * cleared and marked dirty.
 *
 * @param e the ext2-handle
 * @param cnode the inode
 * @return 0 on success
 */
int ext2_inode_destroy(sExt2 *e,sExt2CInode *cnode);

/**
 * Determines which block should be read from disk for <block> of the given inode.
 * That means you give a linear block-number and this function figures out in which block
 * it's stored on the disk. If there is no block yet, it will create it.
 * Note that cnode will not be marked dirty!
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @param block the linear-block-number
 * @return the block to fetch from disk
 */
block_t ext2_inode_reqDataBlock(sExt2 *e,sExt2CInode *cnode,block_t block);

/**
 * Determines which block should be read from disk for <block> of the given inode.
 * That means you give a linear block-number and this function figures out in which block
 * it's stored on the disk. If there is no block yet, it returns 0.
 *
 * @param e the ext2-handle
 * @param cnode the cached inode
 * @param block the linear-block-number
 * @return the block to fetch from disk
 */
block_t ext2_inode_getDataBlock(sExt2 *e,const sExt2CInode *cnode,block_t block);

#if DEBUGGING

/**
 * Prints the given inode
 *
 * @param inode the inode
 */
void ext2_inode_print(sExt2Inode *inode);

#endif
