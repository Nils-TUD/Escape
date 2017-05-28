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
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "../modules.h"

#define PACKET_COUNT	10000

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000};
static char buffer[0x40000];

int mod_writenull(A_UNUSED int argc,A_UNUSED char **argv) {
	uint64_t times[ARRAY_SIZE(sizes)] = {0};
	int fd = open("/dev/null",O_WRONLY);
	if(fd < 0) {
		printe("Unable to open /dev/null");
		return 1;
	}

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t start = rdtsc();
		for(int i = 0; i < PACKET_COUNT; ++i) {
			if(write(fd,buffer,sizes[s]) != (ssize_t)sizes[s])
				printe("write failed");
		}
		times[s] = rdtsc() - start;
	}
	close(fd);

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		printf("per-msg=%5Lu throughput=%Lu MB/s (%db packets)\n",
		       times[s] / PACKET_COUNT,
		       (sizes[s] * PACKET_COUNT) / tsctotime(times[s]),sizes[s]);
	}
	return 0;
}
