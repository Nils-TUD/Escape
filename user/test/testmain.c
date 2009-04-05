/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/fileio.h>
#include <esc/debug.h>
#include <esc/io.h>
#include <stdlib.h>
#include <string.h>

#define BUF_SIZE 4
#define COUNT 1000000

static u8 buffer[BUF_SIZE];

int main(void) {
	/*

	tFD fd;
	u8 buffer[1024];

	fd = open("file:/zeros",IO_READ);
	if(fd < 0) {
		printe("Unable to open file:/zeros");
		return EXIT_FAILURE;
	}

	dbg_startTimer();
	while(read(fd,buffer,1024) > 0);
	dbg_stopTimer("Reading took ");

	close(fd);*/

	tFD fd;
	u32 i;
	u64 start;
	u64 total;
	u32 *ptr;
	u32 t;

	createNode("system:/test");

	fd = open("system:/test",IO_READ | IO_WRITE);

	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		write(fd,buffer,sizeof(buffer));
		seek(fd,0);
	}

	total = cpu_rdtsc() - start;
	ptr = (u32*)&total;
	printf("Write of %d bytes took %08x%08x\n",COUNT * sizeof(buffer),*(ptr + 1),*ptr);
	printf("%d bytes per second\n",(COUNT * sizeof(buffer)) / (getTime() - t));

	t = getTime();
	start = cpu_rdtsc();
	for(i = 0; i < COUNT; i++) {
		read(fd,buffer,sizeof(buffer));
		seek(fd,0);
	}

	total = cpu_rdtsc() - start;
	ptr = (u32*)&total;
	printf("Read of %d bytes took %08x%08x\n",COUNT * sizeof(buffer),*(ptr + 1),*ptr);
	printf("%d bytes per second\n",(COUNT * sizeof(buffer)) / (getTime() - t));

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
