/**
 * $Id: cache.h 189 2011-04-25 17:41:52Z nasmussen $
 */

#ifndef CACHE_H_
#define CACHE_H_

#include "common.h"

// our caches
#define CACHE_INSTR			0
#define CACHE_DATA			1

// cache-data-structures
typedef struct {
	octa tag;
	bool dirty;
	octa *data;
} sCacheBlock;

typedef struct {
	const char *name;
	size_t blockNum;
	size_t blockSize;
	octa tagMask;
	octa lastAddr;
	sCacheBlock *lastBlock;
	sCacheBlock *blocks;
} sCache;

/**
 * Initializes the caches.
 */
void cache_init(void);

/**
 * Resets the caches (removes all entries).
 */
void cache_reset(void);

/**
 * Shuts the caches down. Free's memory
 */
void cache_shutdown(void);

/**
 * Reads an octa from the given cache and address. If it is in the cache, it will simply return
 * the value. If not, it will fetch the corresponding cache-block from the bus and put it into
 * the cache.
 * Note that devices are always uncached!
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @param addr the physical address
 * @param flags control the behaviour:
 * 	MEM_SIDE_EFFECTS: if disabled, the state of the cache doesn't change and events are not fired
 *  MEM_UNCACHED: if enabled, the data is not put into the cache if not already present
 * @return the read octa
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not readable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
octa cache_read(int cache,octa addr,int flags);

/**
 * Writes an octa to the given cache and address. If it is in the cache, it will simply overwrite
 * the value. If not, it will fetch the corresponding cache-block from the bus and put it into the
 * cache.
 * Note that devices are always uncached!
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @param addr the physical address
 * @param flags control the behaviour:
 * 	MEM_SIDE_EFFECTS: if disabled, no events are fired; the state is always changed
 *  MEM_UNCACHED: if enabled, the data is not put into the cache if not already present
 *  MEM_UNINITIALIZED: if enabled, the data will not be fetched from the bus if not already present,
 *    but it will simply be zero'd
 * @throws TRAP_NONEX_MEMORY if the memory at <addr> does not exist or is not writable
 * @throws TRAP_NONEX_MEMORY if a cache-block has to be flushed first and that fails
 */
void cache_write(int cache,octa addr,octa data,int flags);

/**
 * Flushes the affected cache-block to memory, if it is dirty.
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @param addr the physical address
 * @throws TRAP_NONEX_MEMORY if flushing the cache-block fails
 */
void cache_flush(int cache,octa addr);

/**
 * Flushes all dirty cache-blocks from the given cache to memory
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @throws TRAP_NONEX_MEMORY if flushing the cache-block fails
 */
void cache_flushAll(int cache);

/**
 * Removes the affected cache-block from the cache. It is NOT flushed to memory first!
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @param addr the physical address
 */
void cache_remove(int cache,octa addr);

/**
 * Removes all cache-blocks from the given cache. They are NOT flushed to memory first!
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 */
void cache_removeAll(int cache);

/**
 * Only for CLI: Returns the cache-instance
 *
 * @param cache the cache: CACHE_INSTR or CACHE_DATA
 * @return the cache-instance
 */
const sCache *cache_getCache(int cache);

/**
 * Only for CLI
 *
 * @param block the cache-block
 * @return true if the cache-block is valid, i.e. is in use
 */
bool cache_isValid(const sCacheBlock *block);

/**
 * Only for CLI: Finds the cache-block for given address
 *
 * @param cache the cache
 * @param addr the physical address
 * @return the cache-block
 */
const sCacheBlock *cache_find(const sCache *cache,octa addr);

#endif /* CACHE_H_ */
