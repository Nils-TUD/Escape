/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>

s32 main(u32 argc,s8 **argv) {
	u32 i;

	UNUSED(argc);
	UNUSED(argv);

	printf("Currently you can use the following commands:\n");
	printf("\tls <dir>\n");
	printf("\techo <string>\n");
	printf("\tps\n");
	printf("\tkill <pid>\n");
	printf("\tmem\n");
	printf("\tcat <file>\n");
	printf("\n");
	printf("You can scroll the screen with pageUp, pageDown, shift+arrowUp, shift+arrowDown\n");
	printf("\n");

	return 0;
}
