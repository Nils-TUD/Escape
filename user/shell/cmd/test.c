/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <heap.h>
#include <string.h>
#include <proc.h>
#include "test.h"

static void signalHandler(u32 arg) {
	printf("Signal handler...got arg 0x%x\n",arg);
}

s32 shell_cmdTest(u32 argc,s8 **argv) {

	/*printf("Testing signal...\n");
	sigTest((u32)signalHandler);
	printf("Back\n");*/

	/*s8 *buffer;
	u32 res;
	tFD fd;

	UNUSED(argc);
	UNUSED(argv);

	fd = open("file:/bla",IO_READ | IO_WRITE);
	printf("Got fd=%d\n",fd);

	buffer = (s8*)malloc(10 * sizeof(s8));
	res = read(fd,buffer,10 * sizeof(s8));
	printf("Read '%s' (%d bytes)\n",buffer,res);
	free(buffer);

	buffer = (string)"Das ist mein string :)";
	res = write(fd,buffer,strlen(buffer) + 1);
	printf("Wrote '%s' (%d bytes)\n",buffer,res);

	printf("Closing file\n");
	close(fd);*/

	return 0;
}
