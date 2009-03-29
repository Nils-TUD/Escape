/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <test.h>
#include "tests/theap.h"
#include "tests/tbufio.h"
#include "tests/tdir.h"
#include "tests/tenv.h"
#include "tests/tmsgs.h"

int main(void) {
	test_register(&tModHeap);
	test_register(&tModBufio);
	test_register(&tModDir);
	test_register(&tModEnv);
	test_register(&tModMsgs);
	test_start();
	return 0;
}
