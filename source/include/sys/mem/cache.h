/**
 * $Id$
 */

#ifndef CACHE_H_
#define CACHE_H_

#include <sys/common.h>

void *cache_alloc(size_t size);
void *cache_calloc(size_t num,size_t size);
void *cache_realloc(void *p,size_t size);
void cache_free(void *p);
size_t cache_getPageCount(void);
size_t cache_getOccMem(void);
size_t cache_getUsedMem(void);
void cache_dbg_print(void);

#endif /* CACHE_H_ */
