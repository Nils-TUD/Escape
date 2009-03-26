/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <dir.h>
#include <io.h>
#include <bufio.h>
#include <env.h>

int main(int argc,char *argv[]) {
	tFD dd;
	char *path;
	sDirEntry *entry;

	if(argc == 1) {
		path = getEnv("CWD");
		if(path == NULL) {
			printf("Unable to get CWD\n");
			return 1;
		}
	}
	else if(argc == 2)
		path = abspath(argv[1]);
	else {
		printf("Usage: %s [<dir>]\n",argv[0]);
		return 1;
	}

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			printf("% 4d %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}
	else {
		printf("Unable to open '%s'\n",path);
		return 1;
	}

	return 0;
}
