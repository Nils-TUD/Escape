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
#include <stdlib.h>
#include "cd.h"

s32 shell_cmdCd(u32 argc,char **argv) {
	char *path;
	tFD fd;
	sFileInfo info;

	if(argc != 2) {
		printf("Usage: cd <directory>\n");
		return EXIT_FAILURE;
	}

	path = abspath(argv[1]);

	/* ensure that the user remains in file: */
	if(strncmp(path,"file:",5) != 0) {
		printf("Unable to change to '%s'\n",path);
		return 1;
	}

	/* retrieve file-info */
	if(getFileInfo(path,&info) < 0) {
		printe("Unable to get file-info for '%s'",path);
		return EXIT_FAILURE;
	}

	/* check if it is a directory */
	if(!MODE_IS_DIR(info.mode)) {
		fprintf(stderr,"%s is no directory\n",path);
		return EXIT_FAILURE;
	}

	/* finally change dir */
	setEnv("CWD",path);
	return EXIT_SUCCESS;
}
