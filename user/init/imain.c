/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <proc.h>

s32 main(void) {
	s32 fd;
	/* wait for fs */
	do {
		fd = open("services:/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
	}
	while(fd < 0);
	close(fd);

	if(fork() == 0) {
		exec("file:/services/keyboard");
		exit(0);
	}
	if(fork() == 0) {
		exec("file:/services/speaker");
		exit(0);
	}
	if(fork() == 0) {
		exec("file:/services/video");
		exit(0);
	}
	if(fork() == 0) {
		exec("file:/services/vterm");
		exit(0);
	}

	/* atm we assume that this process is the first one (pid=0) and all other
	 * are childs or this one. (inheritance of fds) */
	/* later init might load the services to load from a file, load the services (fork & exec)
	 * and perhaps more. */

	/*u32 i;
	for(i = 0; i < 1000; i++)
		yield();

	if(fork() == 0) {
		exec("file:/task2.bin");
		printf("We should not reach this...\n");
		exit(0);
	}*/

	/* create stdin, stdout and stderr */
	initIO();

	if(fork() == 0) {
		exec("file:/apps/shell");
		exit(0);
	}

	/* loop forever, and don't waste too much cpu-time */
	/* TODO we should improve this some day ;) */
	while(1) {
		yield();
	}
	return 0;
}
