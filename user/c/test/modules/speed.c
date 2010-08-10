/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <stdio.h>
#include <esc/dir.h>
#include <esc/debug.h>
#include <esc/date.h>
#include <errors.h>
#include "speed.h"

/*#define BUF_SIZE	4096 * 32
#define COUNT		18000*/
#define BUF_SIZE	4096 * 32
#define COUNT		30000

static u8 buffer[BUF_SIZE];

int mod_speed(int argc,char *argv[]) {
	tFD fd;
	u64 start;
	uLongLong total;
	u64 i,diff,t;
	char rpath[MAX_PATH_LEN] = "/system/test";

	if(argc > 2)
		abspath(rpath,MAX_PATH_LEN,argv[2]);

	fd = open(rpath,IO_READ | IO_WRITE | IO_CREATE | IO_TRUNCATE);
	if(fd < 0)
		error("open");

	printf("Testing speed of read/write from/to '%s'\n",rpath);
	printf("Transferring %u MiB in chunks of %d bytes\n",(u32)(((u64)COUNT * BUF_SIZE) / (1024 * 1024)),
			BUF_SIZE);
	printf("\n");

#if 0
	/* write */
	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		if(write(fd,buffer,sizeof(buffer)) < 0) {
			printe("write");
			break;
		}
		if(seek(fd,0,SEEK_SET) < 0) {
			printe("seek");
			break;
		}
		if(i % (COUNT / 1000) == 0) {
			diff = getTime() - t;
			printf("\rWriting with	%07d KiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / 1024));
			flush();
		}
	}

	total.val64 = cpu_rdtsc() - start;
	diff = getTime() - t;
	printf("\n");
	printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
	printf("Speed:			%07d KiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / 1024));
	printf("\n");
#endif

	/* read */
	debug();
	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		if(RETRY(read(fd,buffer,sizeof(buffer))) < 0) {
			printe("read");
			break;
		}
#if 0
		if(seek(fd,0,SEEK_SET) < 0) {
			printe("seek");
			break;
		}
		if(i % (COUNT / 1000) == 0) {
			diff = getTime() - t;
			printf("\rReading with	%07d KiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / 1024));
			flush();
		}
#endif
	}
	debug();

	total.val64 = cpu_rdtsc() - start;
	diff = getTime() - t;
	printf("\n");
	printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
	printf("Speed:			%07d KiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / (double)diff) / 1024));

	close(fd);

	return EXIT_SUCCESS;
}
