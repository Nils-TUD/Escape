/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <test.h>
#include "tests/theap.h"
#include "tests/tbufio.h"

int main(int argc,char **argv) {
	test_register(&tModHeap);
	test_register(&tModBufio);
	test_start();
	return 0;
}
