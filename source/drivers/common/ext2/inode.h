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
#include <fs/ext2/ext2.h>
#include <fs/blockcache.h>

class Ext2FileSystem;
struct Ext2CInode;
struct FSUser;

class Ext2INode {
	Ext2INode() = delete;

public:
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
	static int create(Ext2FileSystem *e,FSUser *u,Ext2CInode *dirNode,Ext2CInode **ino,bool isDir);

	/**
	 * Sets the mode of the given inode to <mode>
	 *
	 * @param e the ext2-handle
	 * @param u the user
	 * @param inodeNo the inode-number
	 * @param mode the new mode
	 * @return 0 on success
	 */
	static int chmod(Ext2FileSystem *e,FSUser *u,inode_t inodeNo,mode_t mode);

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
	static int chown(Ext2FileSystem *e,FSUser *u,inode_t inodeNo,uid_t uid,gid_t gid);

	/**
	 * Destroys the given inode. That means the inode will be marked as free in the bitmap,
	 * cleared and marked dirty.
	 *
	 * @param e the ext2-handle
	 * @param cnode the inode
	 * @return 0 on success
	 */
	static int destroy(Ext2FileSystem *e,Ext2CInode *cnode);

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
	static block_t reqDataBlock(Ext2FileSystem *e,Ext2CInode *cnode,block_t block) {
		return doGetDataBlock(e,cnode,block,true);
	}

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
	static block_t getDataBlock(Ext2FileSystem *e,const Ext2CInode *cnode,block_t block) {
		return doGetDataBlock(e,(Ext2CInode*)cnode,block,false);
	}

#if DEBUGGING

	/**
	 * Prints the given inode
	 *
	 * @param inode the inode
	 */
	static void print(Ext2Inode *inode);

#endif

private:
	/**
	 * Performs the actual get-block-request. If <req> is true, it will allocate a new block, if
	 * necessary. In this case cnode may be changed. Otherwise no changes will be made.
	 */
	static block_t doGetDataBlock(Ext2FileSystem *e,Ext2CInode *cnode,block_t block,bool req);
	/**
	 * Puts a new block in cblock->buffer if cblock->buffer[index] is 0. Marks the cblock dirty,
	 * if necessary. Sets <added> to true or false, depending on whether a block was allocated.
	 */
	static int extend(Ext2FileSystem *e,Ext2CInode *cnode,CBlock *cblock,size_t index,bool *added);
};
