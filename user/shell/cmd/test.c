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
	s32 fd;
	u32 count;
	s8 buffer[129];
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

	string path = (string)"file:/bigfile";
	if(argc == 2)
		path = argv[1];

	printf("Opening '%s'\n",path);
	fd = open(path,IO_READ | IO_WRITE);
	if(fd < 0)
		printLastError();
	else {
		printf("Got fd=%d\n",fd);

		printf("Reading...\n");
		while((count = read(fd,buffer,128)) > 0) {
			*(buffer + count) = '\0';
			printf("%s",buffer);
		}
		printf("\nFinished\n");
		close(fd);
	}

	/*buffer = (string)"Das ist mein string :)";
	res = write(fd,buffer,strlen(buffer) + 1);
	printf("Wrote '%s' (%d bytes)\n",buffer,res);

	printf("Closing file\n");
	close(fd);*/

	return 0;
}
