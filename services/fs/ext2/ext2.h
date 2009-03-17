/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef EXT2_H_
#define EXT2_H_

#include <common.h>

typedef struct sExt2 sExt2;

#include "superblock.h"
#include "blockgroup.h"

/* TODO temporary */
#define printf	debugf

#define SECTOR_SIZE							512
#define BLOCK_SIZE(e)						(SECTOR_SIZE << ((e)->superBlock.logBlockSize + 1))
#define BLOCKS_TO_SECS(e,x)					((x) << ((e)->superBlock.logBlockSize + 1))
#define SECS_TO_BLOCKS(e,x)					((x) >> ((e)->superBlock.logBlockSize + 1))

/* number of direct blocks in the inode */
#define EXT2_DIRBLOCK_COUNT					12

struct sExt2 {
	u8 drive;
	u8 partition;
	tFD ataFd;
	sSuperBlock superBlock;
	sBlockGroup *groups;
};

/**
 * Inits the given ext2-filesystem
 *
 * @param e the ext2-data
 * @return true if successfull
 */
bool ext2_init(sExt2 *e);

/**
 * Destroys the given ext2-filesystem
 */
void ext2_destroy(sExt2 *e);

#endif /* EXT2_H_ */
