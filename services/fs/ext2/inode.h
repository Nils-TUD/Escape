/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef INODE_H_
#define INODE_H_

#include <common.h>
#include "ext2.h"

#if DEBUGGING

/**
 * Prints the given inode
 *
 * @param inode the inode
 */
void ext2_dbg_printInode(sInode *inode);

#endif

#endif /* INODE_H_ */
