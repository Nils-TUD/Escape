/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef BLOCKCACHE_H_
#define BLOCKCACHE_H_

#include <common.h>
#include "ext2.h"

/**
 * Inits the block-cache
 *
 * @param e the ext2-handle
 */
void ext2_bcache_init(sExt2 *e);

/**
 * Requests the block with given number
 *
 * @param e the ext2-handle
 * @param blockNo the block to fetch from disk or from the cache
 * @return the block (in a buffer on the heap)
 */
u8 *ext2_bcache_request(sExt2 *e,u32 blockNo);

#endif /* BLOCKCACHE_H_ */
