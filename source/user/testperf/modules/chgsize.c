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
#include <sys/mman.h>
#include <sys/time.h>
#include <stdio.h>

#include "../modules.h"

#define TEST_COUNT		200

int mod_chgsize(A_UNUSED int argc,A_UNUSED char **argv) {
	uint64_t start,end;
	ssize_t counts[] = {1,2,4,8,16,32};
	for(size_t j = 0; j < ARRAY_SIZE(counts); ++j) {
		start = rdtsc();
		for(int i = 0; i < TEST_COUNT; ++i) {
			void *res = chgsize(counts[j]);
			if(res == NULL)
				printe("chgsize(%zd) failed",counts[j]);
		}
		end = rdtsc();
		printf("chgsize(%3zd): %Lu cycles (%Lu cycles/call)\n",counts[j],end - start,(end - start) / TEST_COUNT);

		start = rdtsc();
		for(int i = 0; i < TEST_COUNT; ++i) {
			void *res = chgsize(-counts[j]);
			if(res == NULL)
				printe("chgsize(%zd) failed",-counts[j]);
		}
		end = rdtsc();
		printf("chgsize(%3zd): %Lu cycles (%Lu cycles/call)\n",-counts[j],end - start,(end - start) / TEST_COUNT);
	}
	return 0;
}
