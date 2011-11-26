/**
 * $Id: tc.c 239 2011-08-30 18:28:06Z nasmussen $
 */

#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "core/tc.h"
#include "core/bus.h"
#include "core/register.h"
#include "core/cpu.h"
#include "core/cache.h"
#include "core/mmu.h"
#include "mmix/mem.h"
#include "event.h"
#include "config.h"
#include "exception.h"

typedef struct {
	sTCEntry *entries;
	octa lastKey;
	size_t last;
	size_t size;
} sTranslationCache;

static int nextIndex;
static sTCEntry *instrTC;
static sTCEntry *dataTC;
static sTranslationCache tcs[] = {
	/* TC_INSTR */	{NULL,-1,-1,0},
	/* TC_DATA */	{NULL,-1,-1,0}
};

void tc_init(void) {
	instrTC = (sTCEntry*)mem_alloc(INSTR_TC_SIZE * sizeof(sTCEntry));
	dataTC = (sTCEntry*)mem_alloc(DATA_TC_SIZE * sizeof(sTCEntry));
	tcs[TC_INSTR].entries = instrTC;
	tcs[TC_INSTR].size = INSTR_TC_SIZE;
	tcs[TC_DATA].entries = dataTC;
	tcs[TC_DATA].size = DATA_TC_SIZE;

	tc_reset();
}

void tc_reset(void) {
	nextIndex = 0;
	// put invalid entries in the TC
	for(size_t i = 0; i < ARRAY_SIZE(tcs); i++) {
		for(size_t j = 0; j < tcs[i].size; j++) {
			tcs[i].entries[j].key = MSB(64);
			tcs[i].entries[j].translation = 0;
			tcs[i].last = -1;
			tcs[i].lastKey = -1;
		}
	}
}

void tc_shutdown(void) {
	mem_free(dataTC);
	mem_free(instrTC);
}

sTCEntry *tc_getByIndex(int tc,size_t index) {
	assert(tc == TC_INSTR || tc == TC_DATA);
	if(index < tcs[tc].size)
		return tcs[tc].entries + index;
	return NULL;
}

sTCEntry *tc_getByKey(int tc,octa key) {
	assert(tc == TC_INSTR || tc == TC_DATA);
	if(tcs[tc].lastKey == key)
		return tcs[tc].entries + tcs[tc].last;
	sTCEntry *entries = tcs[tc].entries;
	size_t size = tcs[tc].size;
	for(size_t i = 0; i < size; i++) {
		if(entries[i].key == key) {
			tcs[tc].last = i;
			tcs[tc].lastKey = key;
			return entries + i;
		}
	}
	return NULL;
}

sTCEntry *tc_set(int tc,octa key,octa translation) {
	assert(tc == TC_INSTR || tc == TC_DATA);
	if(tcs[tc].size == 0)
		return NULL;
	size_t index = nextIndex++ % tcs[tc].size;
	sTCEntry *e = tcs[tc].entries + index;
	if(tcs[tc].lastKey == e->key)
		tcs[tc].lastKey = -1;
	e->key = key;
	e->translation = translation;
	return e;
}

void tc_removeAll(int tc) {
	assert(tc == TC_INSTR || tc == TC_DATA);
	sTCEntry *entries = tcs[tc].entries;
	size_t size = tcs[tc].size;
	for(size_t i = 0; i < size; i++)
		tc_remove(tc,entries + i);
}

void tc_remove(int tc,sTCEntry *e) {
	assert(e != NULL);
	if(tcs[tc].lastKey == e->key)
		tcs[tc].lastKey = -1;
	e->key = MSB(64);
	e->translation = 0;
}

