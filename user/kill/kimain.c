/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/signals.h>
#include <string.h>

int main(int argc,char **argv) {
	tPid pid;
	if(argc != 2) {
		printf("Usage: %s <pid>\n",argv[0]);
		return 1;
	}

	pid = atoi(argv[1]);
	if(pid > 0) {
		if(sendSignalTo(pid,SIG_KILL,0) < 0) {
			printLastError();
			return 1;
		}
	}
	else if(strcmp(argv[1],"0") != 0)
		printf("Unable to kill process with pid '%s'\n",argv[1]);
	else
		printf("You can't kill 'init'\n");

	return 0;
}
