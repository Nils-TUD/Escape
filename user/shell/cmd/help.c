/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include "help.h"

s32 shell_cmdHelp(u32 argc,s8 **argv) {
	UNUSED(argc);
	UNUSED(argv);

	printf("Currently you can use the following commands:\n");
	printf("\tcat <file>\n");
	printf("\tkill <pid>\n");
	printf("\tls <dir>\n");
	printf("\tmem\n");
	printf("\tps\n");
	printf("\techo <string1> <string2>, ...\n");
	printf("\tenv [<name>|<name>=<value>]\n");
	printf("\thelp\n");
	printf("\tcd <dir>\n");
	printf("\tpwd\n");

	printf("\n");
	printf("You can scroll the screen with pageUp, pageDown, shift+arrowUp, shift+arrowDown\n");
	printf("\n");

	return 0;
}
