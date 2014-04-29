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

class Ext2Bitmap {
	Ext2Bitmap() = delete;

public:
	/**
	 * Allocates a new inode for the given directory-inode. It will be tried to allocate an inode in
	 * the same block-group
	 *
	 * @param e the ext2-fs
	 * @param dirNode the directory-inode
	 * @param isDir whether the inode should be an directory
	 * @return the inode-number or 0 if failed
	 */
	static inode_t allocInode(Ext2FileSystem *e,Ext2CInode *dirInode,bool isDir);

	/**
	 * Free's the given inode-number
	 *
	 * @param e the ext2-fs
	 * @param ino the inode-number
	 * @param isDir whether the inode is an directory
	 * @return 0 on success
	 */
	static int freeInode(Ext2FileSystem *e,inode_t ino,bool isDir);

	/**
	 * Allocates a new block for the given inode. It will be tried to allocate a block in the same
	 * block-group.
	 *
	 * @param e the ext2-fs
	 * @param inode the inode
	 * @return the block-number or 0 if failed
	 */
	static block_t allocBlock(Ext2FileSystem *e,Ext2CInode *inode);

	/**
	 * Free's the given block-number
	 *
	 * @param e the ext2-fs
	 * @param blockNo the block-number
	 * @return 0 on success
	 */
	static int freeBlock(Ext2FileSystem *e,block_t blockNo);

private:
	static inode_t allocInodeIn(Ext2FileSystem *e,block_t groupStart,Ext2BlockGrp *group,bool isDir);
	static block_t allocBlockIn(Ext2FileSystem *e,block_t groupStart,Ext2BlockGrp *group);
};
