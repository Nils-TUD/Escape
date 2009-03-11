/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include "echo.h"

s32 cmdEcho(u32 argc,s8 **argv) {
	u32 i;
	for(i = 1; i < argc; i++) {
		printf("%s ",argv[i]);
	}
	printf("\n");
	return 0;
}
