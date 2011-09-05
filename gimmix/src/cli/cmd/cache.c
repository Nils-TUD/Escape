/**
 * $Id: cache.c 235 2011-06-16 08:37:02Z nasmussen $
 */

#include <string.h>

#include "common.h"
#include "cli/cmds.h"
#include "cli/cmd/cache.h"
#include "core/cache.h"
#include "mmix/io.h"

static void printEntries(int cache,const sASTNode *addrs);
static void printCache(int cache);
static void printCacheBlock(int cache,const sCacheBlock *b);

void cli_cmd_ic(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);

	if(argc == 1)
		printEntries(CACHE_INSTR,argv[0]);
	else
		printCache(CACHE_INSTR);
}

void cli_cmd_dc(size_t argc,const sASTNode **argv) {
	if(argc > 1)
		cmds_throwEx(NULL);

	if(argc == 1)
		printEntries(CACHE_DATA,argv[0]);
	else
		printCache(CACHE_DATA);
}

static void printEntries(int cache,const sASTNode *addrs) {
	const sCache *c = cache_getCache(cache);
	bool finished = false;
	for(octa offset = 0; ; offset += sizeof(octa)) {
		sCmdArg addr = cmds_evalArg(addrs,ARGVAL_INT,offset,&finished);
		if(finished)
			break;
		const sCacheBlock *b = cache_find(c,addr.d.integer);
		if(b)
			printCacheBlock(cache,b);
		else
			mprintf("- no entry for address %OX -\n",addr.d.integer);
	}
}

static void printCache(int cache) {
	const sCache *c = cache_getCache(cache);
	mprintf("%s (%d bytes in %d blocks, %d bytes each):\n",
			c->name,c->blockNum * c->blockSize,c->blockNum,c->blockSize);
	for(size_t b = 0; b < c->blockNum; b++) {
		if(cache_isValid(c->blocks + b))
			printCacheBlock(cache,c->blocks + b);
	}
}

static void printCacheBlock(int cache,const sCacheBlock *b) {
	const sCache *c = cache_getCache(cache);
	mprintf("%012OX: ",b->tag);
	for(size_t i = 0; i < c->blockSize / sizeof(octa); i++) {
		octa val = cache_read(cache,b->tag + i * sizeof(octa),false);
		if(i > 0 && (i % 4) == 0)
			mprintf("\n%12s  ","");
		mprintf("%016OX ",val);
	}
	mprintf("%c\n",b->dirty ? '*' : ' ');
}
