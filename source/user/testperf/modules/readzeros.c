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
#include <esc/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "../modules.h"

#define PACKET_SIZE		0x4000
#define PACKET_COUNT	100000

static char buffer[PACKET_SIZE];

int mod_readzeros(int argc,char **argv) {
	bool useshm = argc < 3 ? true : strcmp(argv[2],"noshm") != 0;
	int fd = open("/dev/zero",IO_READ);
	if(fd < 0) {
		printe("Unable to open /dev/zero");
		return 1;
	}

	void *buf = buffer;
	ulong name;
	if(useshm && sharebuf(fd,PACKET_SIZE,&buf,&name,0) < 0)
		printe("Unable to share buffer");

	uint64_t start = rdtsc();
	for(int i = 0; i < PACKET_COUNT; ++i) {
		if(read(fd,buf,PACKET_SIZE) != PACKET_SIZE)
			printe("read failed");
	}
	uint64_t end = rdtsc();

	if(useshm)
		destroybuf(buf,name);
	close(fd);

	printf("cycles=%12Lu per-msg=%5Lu throughput=%Lu MB/s (%db packets)\n",
	       end - start,(end - start) / PACKET_COUNT,
	       (PACKET_SIZE * PACKET_COUNT) / tsctotime(end - start),PACKET_SIZE);
	return 0;
}
