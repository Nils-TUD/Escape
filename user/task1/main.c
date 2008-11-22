/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>

s32 main(void) {
	/*u16 pid = fork();
	if(pid == 0) {
		debugf("I am the child with pid %d\n",getpid());
	}
	else if(pid == -1) {
		debugf("Fork failed\n");
	}
	else {
		debugf("I am the parent, created child %d\n",pid);
	}

	while(true) {
		debugf("Hi, my pid is %d, parent is %d\n",getpid(),getppid());
	}*/

	s32 fd = open("/system/services",IO_READ | IO_WRITE), fd2, fd3, fd4;
	if(fd < 0)
		printLastError();
	else {
		if((fd2 = open("/system/services",IO_WRITE)) < 0)
			printLastError();
		else
			close(fd2);

		if((fd3 = open("/system/services",IO_READ)) < 0)
			printLastError();
		else
			close(fd3);

		if((fd4 = open("/..",IO_READ)) < 0)
			printLastError();
		else
			close(fd4);

		close(fd);
	}
	return fd;
}
