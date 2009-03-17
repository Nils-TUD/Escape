/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef REQUEST_H_
#define REQUEST_H_

#include <common.h>
#include "ext2.h"

/**
 * Reads <secCount> sectors at <lba> into the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param lba the start-sector
 * @param secCount the number of sectors
 * @return true if successfull
 */
bool ext2_readSectors(sExt2 *e,u8 *buffer,u64 lba,u16 secCount);

/**
 * Reads <blockCount> blocks at <start> into the given buffer
 *
 * @param e the ext2-handle
 * @param buffer the buffer
 * @param start the start-block
 * @param blockCount the number of blocks
 * @return true if successfull
 */
bool ext2_readBlocks(sExt2 *e,u8 *buffer,u32 start,u16 blockCount);

#endif /* REQUEST_H_ */
