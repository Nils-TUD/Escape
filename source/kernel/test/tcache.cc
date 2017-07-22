/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <mem/cache.h>
#include <sys/test.h>
#include <common.h>
#include <video.h>

#include "testutils.h"

/* forward declarations */
static void test_cache();
static void test_cache_1();
static void test_cache_2();

static const uint TEST_COUNT    = 1000;
static size_t sizes[] = {4,8,16,32,64,128,256,512,1024};

/* our test-module */
sTestModule tModCache = {
	"Cache",
	&test_cache
};

static void test_cache() {
	test_cache_1();
	test_cache_2();
}

static void test_cache_1(void) {
	test_caseStart("Performance of n*(malloc+free)");

	uint64_t atimes[ARRAY_SIZE(sizes)];
	uint64_t ftimes[ARRAY_SIZE(sizes)];

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t atotal = 0, ftotal = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = CPU::rdtsc();
			void *p = Cache::alloc(sizes[s]);
			atotal += CPU::rdtsc() - start;

			start = CPU::rdtsc();
			Cache::free(p);
			ftotal += CPU::rdtsc() - start;
		}
		atimes[s] = atotal;
		ftimes[s] = ftotal;
	}

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		tprintf("malloc(%4zu): %Lu cycles/call\n",sizes[s],atimes[s] / TEST_COUNT);
		tprintf("  free(%4zu): %Lu cycles/call\n",sizes[s],ftimes[s] / TEST_COUNT);
	}

	test_caseSucceeded();
}

static void test_cache_2(void) {
	test_caseStart("Performance of n*malloc + n*free)");

	uint64_t atimes[ARRAY_SIZE(sizes)];
	uint64_t ftimes[ARRAY_SIZE(sizes)];
	void **areas = (void**)Cache::alloc(sizeof(void*) * TEST_COUNT);

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t total = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = CPU::rdtsc();
			areas[i] = Cache::alloc(sizes[s]);
			total += CPU::rdtsc() - start;
		}
		atimes[s] = total;

		total = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = CPU::rdtsc();
			Cache::free(areas[i]);
			total += CPU::rdtsc() - start;
		}
		ftimes[s] = total;
	}

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		tprintf("malloc(%4zu): %Lu cycles/call\n",sizes[s],atimes[s] / TEST_COUNT);
		tprintf("  free(%4zu): %Lu cycles/call\n",sizes[s],ftimes[s] / TEST_COUNT);
	}

	Cache::free(areas);

	test_caseSucceeded();
}
