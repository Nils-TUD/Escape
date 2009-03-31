/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/proc.h>
#include <stdlib.h>

int main(int argc,char **argv) {
	if(exec(0x12345678,NULL) < 0) {
		printe("Exec failed");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
