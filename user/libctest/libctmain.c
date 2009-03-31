/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <stdlib.h>
#include <test.h>

#include "tests/theap.h"
#include "tests/tfileio.h"
#include "tests/tdir.h"
#include "tests/tenv.h"
#include "tests/tmsgs.h"

int main(void) {
	test_register(&tModHeap);
	test_register(&tModFileio);
	test_register(&tModDir);
	test_register(&tModEnv);
	test_register(&tModMsgs);
	test_start();
	return EXIT_SUCCESS;
}
