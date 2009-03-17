/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef BLOCKGROUP_H_
#define BLOCKGROUP_H_

#include <common.h>
#include "ext2.h"
#include "inode.h"

typedef struct {
	/* block id of the first block of the "block bitmap" for the group represented. */
	u32 blockBitmap;
	/* block id of the first block of the "inode bitmap" for the group represented. */
	u32 inodeBitmap;
	/* block id of the first block of the "inode table" for the group represented. */
	u32 inodeTable;
	u16 freeBlockCount;
	u16 freeInodeCount;
	/* the number of inodes allocated to directories for the represented group. */
	u16 usedDirCount;
	/* padding */
	u16 : 16;
	/* reserved */
	u32 : 32;
	u32 : 32;
	u32 : 32;
} __attribute__((packed)) sBlockGroup;

/**
 * Retrieves the inode in the given group from the given inode-number
 *
 * @param e the ext2-handle
 * @param group the block-group
 * @param no the inode-number
 * @return the inode or NULL
 */
sInode *ext2_getInodeInGroup(sExt2 *e,sBlockGroup *group,tInodeNo no);

#endif /* BLOCKGROUP_H_ */
