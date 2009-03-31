/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/env.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include "pwd.h"

s32 shell_cmdPwd(u32 argc,char **argv) {
	char *path;
	UNUSED(argc);
	UNUSED(argv);

	path = getEnv("CWD");
	if(path == NULL) {
		printf("Unable to get CWD\n");
		return 1;
	}

	printf("%s\n",path);
	return 0;
}
