/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef INODE_H_
#define INODE_H_

#include <common.h>
#include "ext2.h"

/**
 * Determines which block should be read from disk for <block> of the given inode.
 * That means you give a linear block-number and this function figures out in which block
 * it's stored on the disk.
 *
 * @param e the ext2-handle
 * @param inode the inode
 * @param block the linear-block-number
 * @return the block to fetch from disk
 */
u32 ext2_getBlockOfInode(sExt2 *e,sInode *inode,u32 block);

#if DEBUGGING

/**
 * Prints the given inode
 *
 * @param inode the inode
 */
void ext2_dbg_printInode(sInode *inode);

#endif

#endif /* INODE_H_ */
