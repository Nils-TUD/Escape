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

#define PACKET_SIZE		(64 * 1024)
#define PACKET_COUNT	100000

static char buffer[PACKET_SIZE];

static void do_read(const char *path,bool useshm) {
	int fd = open(path,IO_READ);
	if(fd < 0) {
		printe("Unable to open %s",path);
		return;
	}

	void *buf = buffer;
	ulong name;
	if(useshm && sharebuf(fd,PACKET_SIZE,&buf,&name,0) < 0)
		printe("Unable to share buffer");

	uint64_t total = 0;
	for(int i = 0; i < PACKET_COUNT; ++i) {
		uint64_t start = rdtsc();
		if(read(fd,buf,PACKET_SIZE) != PACKET_SIZE) {
			printe("read of %s failed",path);
			return;
		}
		total += rdtsc() - start;

		if(seek(fd,SEEK_SET,0) < 0) {
			printe("seek in %s failed",path);
			return;
		}
	}

	if(useshm)
		destroybuf(buf,name);
	close(fd);

	printf("%-16s: cycles=%12Lu per-read=%5Lu throughput=%Lu MB/s (%db packets)\n",
	       path,total,total / PACKET_COUNT,
	       ((uint64_t)PACKET_SIZE * PACKET_COUNT) / tsctotime(total),PACKET_SIZE);
	fflush(stdout);
}

int mod_reading(int argc,char **argv) {
	bool useshm = argc < 3 ? true : strcmp(argv[2],"noshm") != 0;
	int fd = create("/system/test",IO_WRITE,0600);
	if(fd < 0) {
		printe("open of /system/test failed");
		return 1;
	}
	if(write(fd,buffer,sizeof(buffer)) != sizeof(buffer)) {
		printe("write failed");
		return 1;
	}
	close(fd);

	do_read("/dev/zero",useshm);
	do_read("/dev/romdisk",useshm);
	do_read("/system/test",false);
	if(unlink("/system/test") < 0)
		printe("Unlink of /system/test failed");
	return 0;
}
