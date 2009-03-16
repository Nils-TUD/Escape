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
#include <signals.h>
#include <debug.h>
#include "test.h"

u32 sigCount = 0;

static void kbHandler(tSig sig) {
	printf("Got signal %d (%d)\n",sig,sigCount++);
}

s32 shell_cmdTest(u32 argc,s8 **argv) {
	u32 target = 10;
	if(argc == 2) {
		target = atoi(argv[1]);
		if(target == 0)
			target = 10;
	}

	sigCount = 0;
	setSigHandler(SIG_INTRPT_KB,kbHandler);
	while(sigCount < target)
		sleep();
	unsetSigHandler(SIG_INTRPT_KB);

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
