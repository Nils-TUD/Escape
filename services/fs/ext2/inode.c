/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include "ext2.h"
#include "inode.h"

sInode *ext2_getInode(sExt2 *e,tInodeNo no) {
	return ext2_getInodeInGroup(e,e->groups + ext2_getInodeGroup(e,no),ext2_getGroupInodeNo(e,no));
}

u32 ext2_getGroupInodeNo(sExt2 *e,tInodeNo no) {
	return no % e->superBlock.inodesPerGroup;
}

u32 ext2_getInodeGroup(sExt2 *e,tInodeNo no) {
	return no / e->superBlock.inodesPerGroup;
}


#if DEBUGGING

void ext2_dbg_printInode(sInode *inode) {
	u32 i;
	printf("\tmode=0x%08x\n",inode->mode);
	printf("\tuid=%d\n",inode->uid);
	printf("\tgid=%d\n",inode->gid);
	printf("\tsize=%d\n",inode->size);
	printf("\taccesstime=%d\n",inode->accesstime);
	printf("\tcreatetime=%d\n",inode->createtime);
	printf("\tmodifytime=%d\n",inode->modifytime);
	printf("\tdeletetime=%d\n",inode->deletetime);
	printf("\tlinkCount=%d\n",inode->linkCount);
	printf("\tblocks=%d\n",inode->blocks);
	printf("\tflags=0x%08x\n",inode->flags);
	printf("\tosd1=0x%08x\n",inode->osd1);
	for(i = 0; i < EXT2_DIRBLOCK_COUNT; i++)
		printf("\tblock%d=%d\n",i,inode->dBlocks[i]);
	printf("\tsinglyIBlock=%d\n",inode->singlyIBlock);
	printf("\tdoublyIBlock=%d\n",inode->doublyIBlock);
	printf("\ttriplyIBlock=%d\n",inode->triplyIBlock);
	printf("\tgeneration=%d\n",inode->generation);
	printf("\tfileACL=%d\n",inode->fileACL);
	printf("\tdirACL=%d\n",inode->dirACL);
	printf("\tfragAddr=%d\n",inode->fragAddr);
	printf("\tosd2=0x%08x%08x%08x%08x\n",inode->osd2[0],inode->osd2[1],inode->osd2[2],inode->osd2[3]);
}

#endif
