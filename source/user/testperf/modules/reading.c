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
#include <sys/stat.h>
#include <sys/thread.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

#define MAX_PACKET_SIZE		(1024 * 1024)
#define PACKET_COUNT		10000

static size_t sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000,0x100000};
static char buffer[MAX_PACKET_SIZE];

static void do_read(const char *path,bool useshm) {
	uint64_t times[ARRAY_SIZE(sizes)] = {0};
	int fd = open(path,O_RDONLY);
	if(fd < 0) {
		printe("Unable to open %s",path);
		return;
	}

	void *buf = buffer;
	ulong name;
	if(useshm && sharebuf(fd,MAX_PACKET_SIZE,&buf,&name,0) < 0)
		printe("Unable to share buffer");

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		uint64_t total = 0;
		for(int i = 0; i < PACKET_COUNT; ++i) {
			uint64_t start = rdtsc();
			if(read(fd,buf,sizes[s]) != (ssize_t)sizes[s]) {
				printe("read of %s failed",path);
				return;
			}
			total += rdtsc() - start;

			if(seek(fd,SEEK_SET,0) < 0) {
				printe("seek in %s failed",path);
				return;
			}
		}
		times[s] = total;
	}

	if(useshm)
		destroybuf(buf,name);
	close(fd);

	for(size_t s = 0; s < ARRAY_SIZE(sizes); ++s) {
		printf("%-16s: per-read=%5Lu throughput=%Lu MB/s (%db packets)\n",
		       path,times[s] / PACKET_COUNT,
		       ((uint64_t)sizes[s] * PACKET_COUNT) / tsctotime(times[s]),sizes[s]);
	}
	fflush(stdout);
}

int mod_reading(int argc,char **argv) {
	bool useshm = argc < 3 ? true : strcmp(argv[2],"noshm") != 0;
	int fd = create("/sys/test",O_WRONLY,0600);
	if(fd < 0) {
		printe("open of /sys/test failed");
		return 1;
	}
	if(write(fd,buffer,sizeof(buffer)) != sizeof(buffer)) {
		printe("write failed");
		return 1;
	}
	close(fd);

	do_read("/dev/zero",useshm);

	int pid;
	if((pid = fork()) == 0) {
		const char *args[] = {"/sbin/ramdisk","/dev/ramdisk","-m","1048576",NULL};
		execv(args[0],args);
	}
	else {
		struct stat info;
		while(stat("/dev/ramdisk",&info) == -ENOENT)
			sleep(50);

		do_read("/dev/ramdisk",useshm);
		kill(pid,SIGTERM);
		waitchild(NULL,-1);
	}

	do_read("/sys/test",false);
	if(unlink("/sys/test") < 0)
		printe("Unlink of /sys/test failed");
	return 0;
}
