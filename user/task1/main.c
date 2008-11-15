/**
 * @version		$Id: debug.c 51 2008-11-15 09:50:24Z nasmussen $
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>

void main(void) {
	while(true) {
		debugf("Hallo, meine pid ist %d\n",1);
	}
}
