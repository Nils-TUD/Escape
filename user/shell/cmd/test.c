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
	/*u32 target = 10;
	if(argc == 2) {
		target = atoi(argv[1]);
		if(target == 0)
			target = 10;
	}

	sigCount = 0;
	setSigHandler(SIG_INTRPT_KB,kbHandler);
	while(sigCount < target)
		sleep();
	unsetSigHandler(SIG_INTRPT_KB);*/

	u8 buffer[1024];
	u32 count;
	u32 res;
	tFD fd;

	for(count = 0; count < 160; count++)
		buffer[count] = 'a' + (count % 26);
	buffer[count] = '\0';
	for(count = 0; count < 1024; count++)
		printf("%s\n",buffer);

	/*string path = (string)"file:/bigfile";
	if(argc == 2)
		path = argv[1];

	printf("Opening '%s'\n",path);
	fd = open(path,IO_READ | IO_WRITE);
	printf("Got fd=%d\n",fd);

	printf("Reading...\n");
	while((count = read(fd,buffer,16)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}
	printf("\nFinished\n");*/

	/*buffer = (string)"Das ist mein string :)";
	res = write(fd,buffer,strlen(buffer) + 1);
	printf("Wrote '%s' (%d bytes)\n",buffer,res);

	printf("Closing file\n");
	close(fd);*/

	return 0;
}
