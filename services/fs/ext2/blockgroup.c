/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <heap.h>
#include <debug.h>
#include <proc.h>
#include "ext2.h"
#include "blockgroup.h"
#include "inode.h"
#include "request.h"

sInode *ext2_getInodeInGroup(sExt2 *e,sBlockGroup *group,tInodeNo no) {
	ASSERT(no < e->superBlock.inodesPerGroup,"Inode %d not in group",no);
	u32 inodesPerBlock = BLOCK_SIZE(e) / sizeof(sInode);

	/* TODO we need an inode-cache.. */
	static sInode *buffer = NULL;
	if(buffer != NULL)
		free(buffer);

	buffer = (sInode*)malloc(BLOCK_SIZE(e));
	if(!ext2_readBlocks(e,(u8*)buffer,group->inodeTable + no / inodesPerBlock,1))
		return NULL;

	return buffer + (no % inodesPerBlock);
}
