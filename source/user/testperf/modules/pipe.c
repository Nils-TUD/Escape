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
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

#define WRITE_COUNT		10000

static void test_pipe(size_t size) {
	int rfd,wfd;
	if(pipe(&rfd,&wfd) < 0) {
		printe("pipe failed");
		return;
	}

	int i;
	uint64_t start,end;
	const char *name;
	void *buf;
	if(fork() == 0) {
		close(wfd);
		if(sharebuf(rfd,size,&buf,0) < 0)
			printe("Unable to share buffer");
		name = "read";
		start = rdtsc();
		for(i = 0; i < WRITE_COUNT; ++i) {
			if(read(rfd,buf,size) < 0) {
				printe("read failed");
				return;
			}
		}
		end = rdtsc();
		destroybuf(buf);
		close(rfd);
	}
	else {
		close(rfd);
		if(sharebuf(wfd,size,&buf,0) < 0)
			printe("Unable to share buffer");
		name = "write";
		start = rdtsc();
		for(i = 0; i < WRITE_COUNT; ++i) {
			if(write(wfd,buf,size) < 0) {
				printe("write failed");
				return;
			}
		}
		end = rdtsc();
		destroybuf(buf);
		close(wfd);
		waitchild(NULL,-1,0);
	}

	printf("[%4d] %s(%3zuK): %6Lu cycles/call, %Lu MB/s\n",
			getpid(),name,size / 1024,(end - start) / WRITE_COUNT,
			(size * WRITE_COUNT) / tsctotime(end - start));
	/* child should exit here */
	if(strcmp(name,"read") == 0)
		exit(0);
}

int mod_pipe(A_UNUSED int argc,A_UNUSED char *argv[]) {
	size_t i, sizes[] = {0x1000,0x2000,0x4000,0x8000,0x10000};
	for(i = 0; i < ARRAY_SIZE(sizes); ++i) {
		fflush(stdout);
		test_pipe(sizes[i]);
	}
	return 0;
}
