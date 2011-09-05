/**
 * $Id: tsysinstrs.c 178 2011-04-16 14:15:23Z nasmussen $
 */

#include "common.h"
#include "core/bus.h"
#include "core/mmu.h"
#include "core/cpu.h"
#include "core/cache.h"
#include "core/tc.h"
#include "core/decoder.h"
#include "core/register.h"
#include "tsysinstrs.h"

static void test_ldunc(void);
static void test_stunc(void);
static void test_preld(void);
static void test_prego(void);
static void test_prest(void);
static void test_syncd(void);
static void test_syncid(void);
static void test_syncFlushCaches(void);
static void test_syncClearTCs(void);
static void test_syncClearCaches(void);

static void syncCaches(bool instr);
static void execInstr(int opcode,octa x,octa y,octa z);

const sTestCase tSysInstrs[] = {
	{"LDUNC (load octa uncached)",test_ldunc},
	{"STUNC (store octa uncached)",test_stunc},
	{"PRELD (preload data)",test_preld},
	{"PREGO (prefetch to go)",test_prego},
	{"PREST (prestore data)",test_prest},
	{"SYNCD (synchronize data)",test_syncd},
	{"SYNCID (synchronize instructions and data)",test_syncid},
	{"SYNC 5 (flush caches)",test_syncFlushCaches},
	{"SYNC 6 (clear TCs)",test_syncClearTCs},
	{"SYNC 7 (clear caches)",test_syncClearCaches},
	{NULL,0},
};

static void test_ldunc(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);
	bus_write(0x1000,0x1234567890ABCDEF,true);
	test_assertTrue(cache_find(dc,0x1000) == NULL);

	reg_set(0,0);
	execInstr(LDUNC,0,0x1000,0);
	test_assertOcta(reg_get(0),0x1234567890ABCDEF);
	test_assertTrue(cache_find(dc,0x1000) == NULL);
}

static void test_stunc(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	cache_removeAll(CACHE_DATA);
	test_assertTrue(cache_find(dc,0x1000) == NULL);

	// not existent in cache
	execInstr(STUNC,0x1234567890ABCDEF,0x1000,0);
	test_assertOcta(bus_read(0x1000,true),0x1234567890ABCDEF);
	test_assertTrue(cache_find(dc,0x1000) == NULL);

	// existent in cache
	mmu_readOcta(0x1000,MEM_SIDE_EFFECTS);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);
	execInstr(STUNC,0xFFFFFFFFFFFFFFFF,0x1000,0);
	if(dc->blockNum > 0)
		test_assertOcta(bus_read(0x1000,true),0x1234567890ABCDEF);
	test_assertTrue(dc->blockNum == 0 || cache_find(dc,0x1000) != NULL);
	test_assertOcta(cache_read(CACHE_DATA,0x1000,MEM_SIDE_EFFECTS),0xFFFFFFFFFFFFFFFF);
}

static void test_preld(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	if(dc->blockNum > 0) {
		for(int i = 0; i <= 16; i++) {
			for(int j = 0; j <= 16; j++) {
				cache_removeAll(CACHE_DATA);
				execInstr(PRELD,i,0,0x1000 + j);
				// all these addresses should be present in cache now
				for(int x = 0; x <= i; x++)
					test_assertTrue(cache_find(dc,0x1000 + j + x) != NULL);
			}
		}
	}
}

static void test_prego(void) {
	const sCache *ic = cache_getCache(CACHE_INSTR);
	if(ic->blockNum > 0) {
		for(int i = 0; i <= 16; i++) {
			for(int j = 0; j <= 16; j++) {
				cache_removeAll(CACHE_INSTR);
				execInstr(PREGO,i,0,0x1000 + j);
				// all these addresses should be present in cache now
				for(int x = 0; x <= i; x++)
					test_assertTrue(cache_find(ic,0x1000 + j + x) != NULL);
			}
		}
	}
}

static void test_prest(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	if(dc->blockNum > 0) {
		// write some stuff to test for uninitialized values in cache
		bus_write(0x1000,0x1234567890ABCDEF,true);
		bus_write(0x1008,0x1234567890ABCDEF,true);
		bus_write(0x1010,0x1234567890ABCDEF,true);
		bus_write(0x1018,0x1234567890ABCDEF,true);
		for(int i = 0; i <= 16; i++) {
			for(int j = 0; j <= 16; j++) {
				cache_removeAll(CACHE_DATA);
				execInstr(PREST,i,0,0x1000 + j);
				for(int x = 0; x <= i; x++) {
					test_assertTrue(cache_find(dc,0x1000 + j + x) != NULL);
					// it should be uninitialized; that means zero in our case
					test_assertOcta(cache_read(CACHE_DATA,0x1000 + j + x,MEM_SIDE_EFFECTS),0);
				}
			}
		}
	}
}

static void test_syncd(void) {
	syncCaches(false);
}

static void test_syncid(void) {
	syncCaches(true);
}

static void test_syncFlushCaches(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	const sCache *ic = cache_getCache(CACHE_INSTR);
	cache_removeAll(CACHE_DATA);
	cache_removeAll(CACHE_INSTR);

	// fill the caches and memory
	for(octa i = 0; i < dc->blockNum * dc->blockSize / sizeof(octa); i++) {
		cache_write(CACHE_DATA,0x1000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS);
		bus_write(0x1000 + i * sizeof(octa),0xFFFFFFFFFFFFFFFF,true);
	}
	for(octa i = 0; i < ic->blockNum * ic->blockSize / sizeof(octa); i++) {
		cache_write(CACHE_INSTR,0x2000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS);
		bus_write(0x2000 + i * sizeof(octa),0xFFFFFFFFFFFFFFFF,true);
	}

	// flush caches
	execInstr(SYNC,0,5,0);

	// check that the data is still in cache but also in memory
	for(octa i = 0; i < dc->blockNum * dc->blockSize / sizeof(octa); i++) {
		test_assertTrue(cache_find(dc,0x1000 + i * sizeof(octa)) != NULL);
		test_assertOcta(bus_read(0x1000 + i * sizeof(octa),true),i);
	}
	for(octa i = 0; i < ic->blockNum * ic->blockSize / sizeof(octa); i++) {
		test_assertTrue(cache_find(ic,0x2000 + i * sizeof(octa)) != NULL);
		test_assertOcta(bus_read(0x2000 + i * sizeof(octa),true),i);
	}
}

static void test_syncClearTCs(void) {
	tc_removeAll(TC_INSTR);
	tc_removeAll(TC_DATA);

	// fill the TCs
	for(octa i = 0; i < INSTR_TC_SIZE; i++)
		tc_set(TC_INSTR,0x1000 * i,0x1234 + i * 0x10000);
	for(octa i = 0; i < DATA_TC_SIZE; i++)
		tc_set(TC_DATA,0x1000 * i,0x1234 + i * 0x10000);

	// clear TCs
	execInstr(SYNC,0,6,0);

	// check that all entries have been removed
	for(octa i = 0; i < INSTR_TC_SIZE; i++)
		test_assertTrue(tc_getByKey(TC_INSTR,0x1000 * i) == NULL);
	for(octa i = 0; i < DATA_TC_SIZE; i++)
		test_assertTrue(tc_getByKey(TC_DATA,0x1000 * i) == NULL);
}

static void test_syncClearCaches(void) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	const sCache *ic = cache_getCache(CACHE_INSTR);
	cache_removeAll(CACHE_DATA);
	cache_removeAll(CACHE_INSTR);

	// fill the caches and memory
	for(octa i = 0; i < dc->blockNum * dc->blockSize / sizeof(octa); i++) {
		cache_write(CACHE_DATA,0x1000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS);
		bus_write(0x1000 + i * sizeof(octa),0xFFFFFFFFFFFFFFFF,true);
	}
	for(octa i = 0; i < ic->blockNum * ic->blockSize / sizeof(octa); i++) {
		cache_write(CACHE_INSTR,0x2000 + i * sizeof(octa),i,MEM_SIDE_EFFECTS);
		bus_write(0x2000 + i * sizeof(octa),0xFFFFFFFFFFFFFFFF,true);
	}

	// clear caches
	execInstr(SYNC,0,7,0);

	// check that all data is gone and not in memory
	for(octa i = 0; i < dc->blockNum * dc->blockSize / sizeof(octa); i++) {
		test_assertTrue(cache_find(dc,0x1000 + i * sizeof(octa)) == NULL);
		test_assertOcta(bus_read(0x1000 + i * sizeof(octa),true),0xFFFFFFFFFFFFFFFF);
	}
	for(octa i = 0; i < ic->blockNum * ic->blockSize / sizeof(octa); i++) {
		test_assertTrue(cache_find(ic,0x2000 + i * sizeof(octa)) == NULL);
		test_assertOcta(bus_read(0x2000 + i * sizeof(octa),true),0xFFFFFFFFFFFFFFFF);
	}
}

static void syncCaches(bool instr) {
	const sCache *dc = cache_getCache(CACHE_DATA);
	const sCache *ic = cache_getCache(CACHE_INSTR);
	octa data[] = {0x1234,0x5678,0x90AB,0xCDEF,0xFFFF};

	for(int t = 0; t < 2; t++) {
		// when in privileged mode, the data is also removed from cache
		cpu_setPC(t == 1 ? MSB(64) : 0x1000);

		for(int i = 0; i <= 16; i++) {
			for(int j = 0; j <= 16; j++) {
				cache_removeAll(CACHE_DATA);
				cache_removeAll(CACHE_INSTR);
				// clear memory
				for(size_t x = 0; x < ARRAY_SIZE(data); x++)
					bus_write(0x1000 + x * sizeof(octa),0,true);
				// write some data into the cache
				for(size_t x = 0; x < ARRAY_SIZE(data); x++) {
					if(instr)
						cache_read(CACHE_INSTR,0x1000 + x * sizeof(octa),MEM_SIDE_EFFECTS);
					cache_write(CACHE_DATA,0x1000 + x * sizeof(octa),data[x],MEM_SIDE_EFFECTS);
				}
				if(dc->blockNum > 0) {
					// should not be present in memory yet
					for(size_t x = 0; x < ARRAY_SIZE(data); x++)
						test_assertOcta(bus_read(0x1000 + x * sizeof(octa),true),0);
				}
				// flush it
				execInstr(instr ? SYNCID : SYNCD,i,0,0x1000 + j);
				// test it
				for(int x = 0; x <= i; x++) {
					if(instr) {
						octa o = bus_read(0x1000 + j + x,true);
						if(t == 1) {
							// data is not flushed to memory
							if(dc->blockNum > 0)
								test_assertOcta(o,0);
							// and removed from all caches
							test_assertTrue(cache_find(dc,0x1000 + j + x) == NULL);
							test_assertTrue(cache_find(ic,0x1000 + j + x) == NULL);
						}
						else {
							// data is flushed to memory and removed from IC
							test_assertOcta(o,data[(j + x) / sizeof(octa)]);
							test_assertTrue(cache_find(ic,0x1000 + j + x) == NULL);
						}
					}
					else {
						// data is flushed to memory and removed from cache in privileged mode
						test_assertOcta(bus_read(0x1000 + j + x,true),data[(j + x) / sizeof(octa)]);
						bool foundInDC = cache_find(dc,0x1000 + j + x) != NULL;
						test_assertTrue(t == 1 || dc->blockNum == 0 ? !foundInDC : foundInDC);
					}
				}
			}
		}
	}
}

static void execInstr(int opcode,octa x,octa y,octa z) {
	sInstrArgs instr;
	instr.x = x;
	instr.y = y;
	instr.z = z;
	const sInstr *desc = dec_getInstr(opcode);
	desc->execute(&instr);
}
