/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/env.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <string.h>
#include "cd.h"

s32 shell_cmdCd(u32 argc,char **argv) {
	char *path;
	tFD fd;

	if(argc != 2) {
		printf("Usage: cd <directory>\n");
		return 1;
	}

	path = abspath(argv[1]);

	/* ensure that the user remains in file: */
	if(strncmp(path,"file:",5) != 0) {
		printf("Unable to change to '%s'\n",path);
		return 1;
	}

	/* check if the path exists */
	if((fd = open(path,IO_READ)) >= 0) {
		close(fd);
		setEnv("CWD",path);
	}
	else {
		printf("Directory '%s' does not exist\n",path);
		return 1;
	}

	return 0;
}
