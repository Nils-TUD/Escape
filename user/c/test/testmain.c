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
#include <esc/fileio.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <esc/service.h>
#include <esc/date.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4096
#define COUNT 100000

static u8 buffer[BUF_SIZE];

int main(void) {
	tFD fd;
	u64 start;
	uLongLong total;
	u32 i,diff,t;

	createNode("system:/test");

	fd = open("system:/test",IO_READ | IO_WRITE);

	printf("Testing speed of read/write to VFS-node\n");
	printf("Transferring %d MiB in chunks of %d bytes\n",(COUNT * BUF_SIZE) / M,BUF_SIZE);
	printf("\n");

	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		write(fd,buffer,sizeof(buffer));
		seek(fd,0);
		if(i % (COUNT / 100) == 0) {
			diff = getTime() - t;
			printf("\rWriting with	%03d MiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
		}
	}

	total.val64 = cpu_rdtsc() - start;
	diff = getTime() - t;
	printf("\n");
	printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
	printf("Speed:			%03d MiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
	printf("\n");

	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		read(fd,buffer,sizeof(buffer));
		seek(fd,0);
		if(i % (COUNT / 100) == 0) {
			diff = getTime() - t;
			printf("\rReading with	%03d MiB/s",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));
		}
	}

	total.val64 = cpu_rdtsc() - start;
	diff = getTime() - t;
	printf("\n");
	printf("Instructions:	%08x%08x\n",total.val32.upper,total.val32.lower);
	printf("Speed:			%03d MiB/s\n",diff == 0 ? 0 : ((i * sizeof(buffer) / diff) / M));

	close(fd);

	/*int c1,c2,c3;
	char line[50];
	char str[] = "- This, a sample string.";
	char *pch;
	s32 res;

	printf("Splitting string \"%s\" into tokens:\n",str);
	pch = strtok(str," ,.-");
	while(pch != NULL) {
		printf("'%s'\n",pch);
		pch = strtok(NULL," ,.-");
	}

	printf("str=%p,%n pch=%p,%n abc=%p,%n\n",str,&c1,pch,&c2,0x1234,&c3);
	printf("c1=%d, c2=%d, c3=%d\n",c1,c2,c3);

	printf("num1: '%-8d', num2: '%8d', num3='%-16x', num4='%-012X'\n",
			100,200,0x1bca,0x12345678);

	printf("num1: '%-+4d', num2: '%-+4d'\n",-100,50);
	printf("num1: '%- 4d', num2: '%- 4d'\n",-100,50);
	printf("num1: '%#4x', num2: '%#4X', num3 = '%#4o'\n",0x123,0x456,0377);
	printf("Var padding: %*d\n",8,-123);
	printf("short: %4hx\n",0x1234);

	while(1) {
		printf("Now lets execute something...\n");
		scanl(line,50);
		res = system(line);
		printf("Result: %d\n",res);
	}*/

	return EXIT_SUCCESS;
}
