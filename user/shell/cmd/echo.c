/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/io.h>
#include <esc/bufio.h>
#include "echo.h"

s32 shell_cmdEcho(u32 argc,char *argv[]) {
	u32 i;
	for(i = 1; i < argc; i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");
	return 0;
}
