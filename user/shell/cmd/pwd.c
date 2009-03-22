/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <env.h>
#include <io.h>

s32 shell_cmdPwd(u32 argc,s8 **argv) {
	s8 *path;
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
