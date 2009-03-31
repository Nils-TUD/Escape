/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/env.h>
#include <stdlib.h>

int main(int argc,char *argv[]) {
	tFD dd;
	char *path;
	sDirEntry *entry;

	if(argc == 1) {
		path = getEnv("CWD");
		if(path == NULL) {
			printe("Unable to get CWD\n");
			return EXIT_FAILURE;
		}
	}
	else if(argc == 2)
		path = abspath(argv[1]);
	else {
		fprintf(stderr,"Usage: %s [<dir>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			printf("% 4d %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}
	else {
		printe("Unable to open '%s'\n",path);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
