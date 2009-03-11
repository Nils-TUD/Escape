/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>
#include <io.h>
#include <dir.h>
#include <string.h>
#include <service.h>
#include <mem.h>
#include <heap.h>

s32 main(void) {
	u32 i;
	/* given the other processes the chance to init */
	for(i = 0; i < 10; i++)
		yield();

	while(1) {
		printf("Type something: ");
		s8 *buffer = malloc(20 * sizeof(s8));
		u16 count = readLine(buffer,20);
		printf("You typed: %s\n",buffer);
	}

	return 0;
}
