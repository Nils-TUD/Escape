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

#include <sys/common.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

static const uint TEST_COUNT    = 10000;
static size_t sizes[] = {4,8,16,32,64,128,256,512,1024};

static void test1(void) {
	uint64_t atimes[ARRAY_SIZE(sizes)];
	uint64_t ftimes[ARRAY_SIZE(sizes)];

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t atotal = 0, ftotal = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = rdtsc();
			void *p = malloc(sizes[s]);
			atotal += rdtsc() - start;

			start = rdtsc();
			free(p);
			ftotal += rdtsc() - start;
		}
		atimes[s] = atotal;
		ftimes[s] = ftotal;
	}

	printf("n*(malloc+free):\n");
	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		printf("malloc(%zu): %Lu cycles/call\n",sizes[s],atimes[s] / TEST_COUNT);
		printf("  free(%zu): %Lu cycles/call\n",sizes[s],ftimes[s] / TEST_COUNT);
	}
}

static void test2(void) {
	uint64_t atimes[ARRAY_SIZE(sizes)];
	uint64_t ftimes[ARRAY_SIZE(sizes)];
	void **areas = (void**)malloc(sizeof(void*) * TEST_COUNT);

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t total = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = rdtsc();
			areas[i] = malloc(sizes[s]);
			total += rdtsc() - start;
		}
		atimes[s] = total;

		total = 0;
		for(uint i = 0; i < TEST_COUNT; ++i) {
			uint64_t start = rdtsc();
			free(areas[i]);
			total += rdtsc() - start;
		}
		ftimes[s] = total;
	}

	printf("n*malloc + n*free:\n");
	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		printf("malloc(%zu): %Lu cycles/call\n",sizes[s],atimes[s] / TEST_COUNT);
		printf("  free(%zu): %Lu cycles/call\n",sizes[s],ftimes[s] / TEST_COUNT);
	}
	free(areas);
}

int mod_heap(A_UNUSED int argc,A_UNUSED char *argv[]) {
	test1();
	test2();
	return 0;
}
