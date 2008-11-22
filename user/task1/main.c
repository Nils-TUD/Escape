/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <string.h>

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

	debugf("Listing directory '/':\n");
	s32 fd = open("/",IO_READ);
	if(fd < 0)
		printLastError();
	else {
		sVFSNode node;
		s8 nameBuf[255];
		s32 c;
		while((c = read(fd,(u8*)&node,sizeof(sVFSNode))) > 0) {
			if((c = read(fd,(u8*)nameBuf,node.nameLen)) > 0) {
				debugf("- %s\n",nameBuf);
			}
		}
		debugf("done\n");

		close(fd);
	}
	return fd;
}
