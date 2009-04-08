/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/fileio.h>
#include <stdlib.h>

int main(void) {
	/*if(exec((const char*)0x12345678,NULL) < 0) {
		printe("Exec failed");
		return EXIT_FAILURE;
	}*/

	u32 i;
	for(i = 0; i < 250; i++) {
		if(fork() == 0) {
			while(1)
				wait(EV_NOEVENT);
		}
	}

	return EXIT_SUCCESS;
}
