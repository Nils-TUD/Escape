/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#ifndef INODECACHE_H_
#define INODECACHE_H_

#include <common.h>
#include "ext2.h"

/**
 * Inits the inode-cache
 *
 * @param e the ext2-handle
 */
void ext2_icache_init(sExt2 *e);

/**
 * Requests the inode with given number. That means if it is in the cache you'll simply get it.
 * Otherwise it is fetched from disk and put into the cache. The references of the cache-inode
 * will be increased.
 *
 * @param e the ext2-handle
 * @param no the inode-number
 * @return the cached node or NULL
 */
sCachedInode *ext2_icache_request(sExt2 *e,tInodeNo no);

/**
 * Releases the given inode. That means the references will be decreased and the inode will be
 * removed from cache if there are no more references.
 *
 * @param e the ext2-handle
 * @param inode the inode
 */
void ext2_icache_release(sExt2 *e,sCachedInode *inode);

/**
 * Prints inode-cache statistics
 */
void ext2_icache_printStats(void);

#endif /* INODECACHE_H_ */
