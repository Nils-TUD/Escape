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

#include <esc/common.h>
#include <esc/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "../modules.h"

#define PACKET_SIZE		0x4000
#define PACKET_COUNT	100000

int mod_writenull(A_UNUSED int argc,A_UNUSED char **argv) {
	static char buffer[PACKET_SIZE];
	int fd = open("/dev/null",IO_WRITE);
	if(fd < 0) {
		printe("Unable to open /dev/null");
		return 1;
	}
	uint64_t start = rdtsc();
	for(int i = 0; i < PACKET_COUNT; ++i) {
		if(write(fd,buffer,sizeof(buffer)) != sizeof(buffer))
			printe("write failed");
	}
	uint64_t end = rdtsc();
	printf("cycles=%12Lu per-msg=%5Lu throughput=%Lu MB/s (%db packets)\n",
	       end - start,(end - start) / PACKET_COUNT,
	       (PACKET_SIZE * PACKET_COUNT) / tsctotime(end - start),PACKET_SIZE);
	close(fd);
	return 0;
}
