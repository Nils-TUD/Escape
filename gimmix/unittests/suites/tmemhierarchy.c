/**
 * $Id: tmemhierarchy.c 202 2011-05-10 10:49:04Z nasmussen $
 */

#include <stdlib.h>

#include "common.h"
#include "core/mmu.h"
#include "core/bus.h"
#include "core/cache.h"
#include "tmemhierarchy.h"
#include "event.h"
#include "mmix/io.h"

static void eventHandler(const sEvArgs *args);
static void test_read(void);
static void test_readSideEffects(void);
static void test_write(void);
static void test_writeSideEffects(void);
static void test_sync(void);
static void test_io(void);

const sTestCase tMemHierarchy[] = {
	{"Reading",test_read},
	{"Reading without side-effects",test_readSideEffects},
	{"Writing",test_write},
	{"Writing without side-effects",test_writeSideEffects},
	{"Syncing",test_sync},
	{"IO-Devices",test_io},
	{NULL,0},
};
static int events = 0;

static void eventHandler(const sEvArgs *args) {
	UNUSED(args);
	events++;
}

static void test_read(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);

	// use caching
	bus_write(0x1000,0x1234567890ABCDEF,true);
	test_assertOcta(mmu_readOcta(0x1000,MEM_SIDE_EFFECTS),0x1234567890ABCDEF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);

	// no caching, already present
	test_assertOcta(mmu_readOcta(0x1000,MEM_SIDE_EFFECTS | MEM_UNCACHED),0x1234567890ABCDEF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);

	// no caching, not yet present
	bus_write(0x2000,0x1234567890ABCDEF,true);
	test_assertOcta(mmu_readOcta(0x2000,MEM_SIDE_EFFECTS | MEM_UNCACHED),0x1234567890ABCDEF);
	test_assertTrue(cache_find(dc,0x2000) == NULL);

	const sCache *ic = cache_getCache(CACHE_INSTR);
	cache_removeAll(CACHE_INSTR);

	// read instruction
	bus_write(0x1000,0x1234567890ABCDEF,true);
	test_assertTetra(mmu_readInstr(0x1000,MEM_SIDE_EFFECTS),0x12345678);
	test_assertTrue(ic->blockNum == 0 || cache_find(ic,0x1000) != NULL);
}

static void test_readSideEffects(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);

	mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);

	// register event-nandler
	ev_register(EV_CACHE_FILL,eventHandler);
	ev_register(EV_CACHE_FLUSH,eventHandler);
	ev_register(EV_DEV_READ,eventHandler);
	ev_register(EV_DEV_WRITE,eventHandler);
	ev_register(EV_VAT,eventHandler);

	// existent in cache
	events = 0;
	test_assertOcta(mmu_readOcta(0x1000,0),0x1234567890ABCDEF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);
	test_assertInt(events,0);

	// non existent in cache
	events = 0;
	bus_write(0x2000,0x1234567890ABCDEF,false);
	test_assertOcta(mmu_readOcta(0x2000,0),0x1234567890ABCDEF);
	test_assertTrue(cache_find(dc,0x2000) == NULL);
	test_assertInt(events,0);

	// unregister them
	ev_unregister(EV_CACHE_FILL,eventHandler);
	ev_unregister(EV_CACHE_FLUSH,eventHandler);
	ev_unregister(EV_DEV_READ,eventHandler);
	ev_unregister(EV_DEV_WRITE,eventHandler);
	ev_unregister(EV_VAT,eventHandler);
}

static void test_write(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);

	// write more than the cache can hold, cached
	for(octa i = 0; i < dc->blockNum * 2; i++) {
		mmu_writeOcta(0x1000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS);
		test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000 + i * sizeof(octa)) != NULL);
		test_assertOcta(cache_read(CACHE_DATA,0x1000 + i * sizeof(octa),MEM_SIDE_EFFECTS),i);
	}
	cache_flushAll(CACHE_DATA);
	// now, all has to be in memory
	for(octa i = 0; i < dc->blockNum * 2; i++)
		test_assertOcta(bus_read(0x1000 + i * sizeof(octa),true),i);

	// write more than the cache can hold, uncached
	cache_removeAll(CACHE_DATA);
	for(octa i = 0; i < dc->blockNum * 2; i++) {
		mmu_writeOcta(0x1000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS | MEM_UNCACHED);
		test_assertTrue(cache_find(dc,0x1000 + i * sizeof(octa)) == NULL);
		test_assertOcta(bus_read(0x1000 + i * sizeof(octa),true),i);
	}

	// uninitialized
	if(dc->blockNum > 0) {
		cache_removeAll(CACHE_DATA);
		bus_write(0x1000,0x1234567890ABCDEF,true);
		mmu_writeTetra(0x1004,0xFFFFFFFF,MEM_SIDE_EFFECTS | MEM_UNINITIALIZED);
		test_assertTrue(cache_find(dc,0x1000) != NULL);
		test_assertOcta(mmu_readOcta(0x1000,MEM_SIDE_EFFECTS),0x00000000FFFFFFFF);
	}
}

static void test_writeSideEffects(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);

	// note that writing has always the side-effect that the data is written. but it won't fire
	// any events

	mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
	mmu_writeOcta(0x1008,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);

	// register event-nandler
	ev_register(EV_CACHE_FILL,eventHandler);
	ev_register(EV_CACHE_FLUSH,eventHandler);
	ev_register(EV_DEV_READ,eventHandler);
	ev_register(EV_DEV_WRITE,eventHandler);
	ev_register(EV_VAT,eventHandler);

	// existent in cache
	events = 0;
	mmu_writeOcta(0x1000,0xFFFFFFFFFFFFFFFF,0);
	test_assertOcta(mmu_readOcta(0x1000,0),0xFFFFFFFFFFFFFFFF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);
	test_assertInt(events,0);

	// existent in cache, tetra
	events = 0;
	mmu_writeTetra(0x1008,0xFFFFFFFF,0);
	test_assertOcta(mmu_readOcta(0x1008,0),0xFFFFFFFF90ABCDEF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1008) != NULL);
	test_assertInt(events,0);

	// non existent in cache
	events = 0;
	mmu_writeOcta(0x2000,0xFFFFFFFFFFFFFFFF,0);
	test_assertOcta(mmu_readOcta(0x2000,0),0xFFFFFFFFFFFFFFFF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x2000) != NULL);
	test_assertInt(events,0);

	// non existent in cache, tetra
	events = 0;
	mmu_writeTetra(0x2008,0xFFFFFFFF,0);
	test_assertOcta(mmu_readOcta(0x2008,0),0xFFFFFFFF00000000);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x2008) != NULL);
	test_assertInt(events,0);

	// unregister them
	ev_unregister(EV_CACHE_FILL,eventHandler);
	ev_unregister(EV_CACHE_FLUSH,eventHandler);
	ev_unregister(EV_DEV_READ,eventHandler);
	ev_unregister(EV_DEV_WRITE,eventHandler);
	ev_unregister(EV_VAT,eventHandler);
}

static void test_sync(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	const sCache *ic = cache_getCache(CACHE_INSTR);
	cache_removeAll(CACHE_DATA);
	cache_removeAll(CACHE_INSTR);

	if(dc->blockNum > 0) {
		bus_write(0x1000,0,true);
		mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
		// not yet in memory
		test_assertOcta(bus_read(0x1000,true),0);

		mmu_syncdOcta(0x1000,false);
		// now it is and still in cache
		test_assertOcta(bus_read(0x1000,true),0x1234567890ABCDEF);
		test_assertTrue(cache_find(dc,0x1000) != NULL);

		// its always octa-aligned
		bus_write(0x1000,0,true);
		mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
		test_assertOcta(bus_read(0x1000,true),0);
		mmu_syncdOcta(0x1007,false);
		test_assertOcta(bus_read(0x1000,true),0x1234567890ABCDEF);
		test_assertTrue(cache_find(dc,0x1000) != NULL);

		// remove it from cache
		bus_write(0x1000,0,true);
		mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
		test_assertOcta(bus_read(0x1000,true),0);
		mmu_syncdOcta(0x1000,true);
		test_assertOcta(bus_read(0x1000,true),0x1234567890ABCDEF);
		test_assertTrue(cache_find(dc,0x1000) == NULL);
	}

	// sync instr with data
	bus_write(0x1000,0,true);
	cache_read(CACHE_INSTR,0x1000,MEM_SIDE_EFFECTS);
	test_assertTetra(mmu_readInstr(0x1000,MEM_SIDE_EFFECTS),0);
	mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
	if(dc->blockNum > 0)
		test_assertOcta(bus_read(0x1000,true),0);
	mmu_syncidOcta(0x1000,false);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);
	test_assertTrue(cache_find(ic,0x1000) == NULL);
	test_assertTetra(mmu_readInstr(0x1000,MEM_SIDE_EFFECTS),0x12345678);

	// syncid drastically
	cache_removeAll(CACHE_INSTR);
	bus_write(0x1000,0,true);
	cache_read(CACHE_INSTR,0x1000,MEM_SIDE_EFFECTS);
	test_assertTetra(mmu_readInstr(0x1000,MEM_SIDE_EFFECTS),0);
	mmu_writeOcta(0x1000,0x1234567890ABCDEF,MEM_SIDE_EFFECTS);
	if(dc->blockNum > 0)
		test_assertOcta(bus_read(0x1000,true),0);
	mmu_syncidOcta(0x1000,true);
	if(dc->blockNum > 0)
		test_assertOcta(bus_read(0x1000,true),0);
	test_assertTrue(cache_find(dc,0x1000) == NULL);
	test_assertTrue(cache_find(ic,0x1000) == NULL);
}

static void test_io(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);

	// io-devices are always uncached (write 0x3 to timer-ctrl-reg)
	mmu_writeOcta(0x8001000000000000,0x3,MEM_SIDE_EFFECTS);
	test_assertTrue(cache_find(dc,0x1000000000000) == NULL);

	test_assertOcta(mmu_readOcta(0x8001000000000000,MEM_SIDE_EFFECTS),0x3);
	test_assertTrue(cache_find(dc,0x1000000000000) == NULL);
}
