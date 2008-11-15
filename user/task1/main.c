/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <debug.h>
#include <proc.h>

void main(void) {
	while(true) {
		debugf("Hi, my pid is %d, parent is %d\n",getpid(),getppid());
	}
}
