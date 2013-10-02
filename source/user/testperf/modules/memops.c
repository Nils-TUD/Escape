/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "memops.h"

typedef void (*memop_func)(void *a,void *b,size_t len);

static void do_test(const char *name,memop_func func);

static void memcpy_func(void *a,void *b,size_t len) {
    memcpy(a,(const void*)b,len);
}
static void memset_func(void *a,A_UNUSED void *b,size_t len) {
    memset(a,0,len);
}

static const size_t AREA_SIZE   = 4096;
static const uint TEST_COUNT    = 1000;

int mod_memops(A_UNUSED int argc,A_UNUSED char *argv[]) {
    do_test("memcpy", memcpy_func);
    do_test("memset", memset_func);
	return 0;
}

static void do_test(const char *name,memop_func func) {
    void *mem = malloc(AREA_SIZE);
    void *buf = malloc(AREA_SIZE);

    {
        uint64_t total = 0;
        for(uint i = 0; i < TEST_COUNT; ++i) {
            uint64_t start = rdtsc();
            func(buf, mem, AREA_SIZE);
            total += rdtsc() - start;
        }
        printf("Aligned %s with %zu bytes: %Lu cycles/call, %Lu MiB/s\n",
               name,AREA_SIZE,total / TEST_COUNT,
               (AREA_SIZE * TEST_COUNT) / tsctotime(total));
    }

    {
        uint64_t total = 0;
        for(uint i = 0; i < TEST_COUNT; ++i) {
            uint64_t start = rdtsc();
            func((char*)buf + 1,(char*)mem + 1,AREA_SIZE - 2);
            total += rdtsc() - start;
        }
        printf("Unaligned %s with %zu bytes: %Lu cycles/call, %Lu MiB/s\n",
               name,AREA_SIZE,total / TEST_COUNT,
               (AREA_SIZE * TEST_COUNT) / tsctotime(total));
    }

    free(buf);
    free(mem);
}
