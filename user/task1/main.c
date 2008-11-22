/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <dir.h>
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
	s32 dd;
	sDir *entry;
	if((dd = opendir("/")) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			debugf("- %d : %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}
	return dd;
}
