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
#include <esc/io.h>
#include <esc/debug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "speed.h"

int mod_speed(int argc,char *argv[]) {
	int fd;
	ullong start;
	ullong total;
	ullong i,diff,t;
	size_t count = 30000;
	size_t bufSize = 4096;
	void *buffer;
	const char *path = "/system/test";

	if(argc > 2)
		path = argv[2];
	if(argc > 3)
		count = atoi(argv[3]);
	if(argc > 4)
		bufSize = atoi(argv[4]);

	if(count == 0)
		error("Got count=0");
	buffer = malloc(bufSize);
	if(!buffer)
		error("malloc");

	fd = open(path,IO_READ | IO_WRITE | IO_CREATE | IO_TRUNCATE);
	if(fd < 0)
		error("open");

	printf("Testing speed of read/write from/to '%s'\n",path);
	printf("Transferring %Lu MiB in chunks of %zu bytes\n",
			(((ullong)count * bufSize) / (1024 * 1024)),bufSize);
	printf("\n");

	/* write */
	t = time(NULL);
	start = cpu_rdtsc();
	for(i = 0; i < count; i++) {
		if(write(fd,buffer,bufSize) < 0) {
			printe("write");
			break;
		}
		if(seek(fd,0,SEEK_SET) < 0) {
			printe("seek");
			break;
		}
		if(count >= 1000 && i % (count / 1000) == 0) {
			diff = time(NULL) - t;
			printf("\rWriting with	%07Lu KiB/s",
					diff == 0 ? 0ULL : ((i * bufSize / diff) / 1024ULL));
			fflush(stdout);
		}
	}

	total = cpu_rdtsc() - start;
	diff = time(NULL) - t;
	printf("\n");
	printf("Instructions:	%016Lx\n",total);
	printf("Speed:			%07Lu KiB/s\n",diff == 0 ? 0ULL : ((i * bufSize / diff) / 1024ULL));
	printf("\n");

	/* read */
	t = time(NULL);
	start = cpu_rdtsc();
	for(i = 0; i < count; i++) {
		if(RETRY(read(fd,buffer,bufSize)) < 0) {
			printe("read");
			break;
		}
		if(seek(fd,0,SEEK_SET) < 0) {
			printe("seek");
			break;
		}
		if(count >= 1000 && i % (count / 1000) == 0) {
			diff = time(NULL) - t;
			printf("\rReading with	%07Lu KiB/s",
					diff == 0 ? 0ULL : ((i * bufSize / diff) / 1024ULL));
			fflush(stdout);
		}
	}

	total = cpu_rdtsc() - start;
	diff = time(NULL) - t;
	printf("\n");
	printf("Instructions:	%016Lx\n",total);
	printf("Speed:			%07Lu KiB/s\n",
			diff == 0 ? 0ULL : ((i * bufSize / diff) / 1024ULL));

	free(buffer);
	close(fd);

	return EXIT_SUCCESS;
}
