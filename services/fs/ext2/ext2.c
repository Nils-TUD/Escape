/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include "ext2.h"
#include "request.h"

bool ext2_init(sExt2 *e) {
	e->ataFd = open("services:/ata",IO_WRITE | IO_READ);
	if((s32)e->ataFd < 0) {
		printLastError();
		return false;
	}

	if(!ext2_readSectors(e,&(e->superBlock),2,1)) {
		close(e->ataFd);
		printLastError();
		return false;
	}

	e->groups = (sBlockGroup*)malloc(BLOCK_SIZE(e));
	/* TODO determine block-number of group-table */
	if(!ext2_readBlocks(e,(u8*)e->groups,2,1)) {
		close(e->ataFd);
		printLastError();
		return false;
	}

	return true;
}

void ext2_destroy(sExt2 *e) {
	free(e->groups);
	close(e->ataFd);
}
