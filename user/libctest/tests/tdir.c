/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <dir.h>
#include <test.h>
#include <stdarg.h>
#include "tdir.h"

/* forward declarations */
static void test_dir(void);

/* our test-module */
sTestModule tModDir = {
	"Dir",
	&test_dir
};

static void test_dir(void) {
	tFD fd;
	sDirEntry *e;
	test_caseStart("Testing opendir, readdir and closedir");

	fd = opendir("file:/bin");
	if(fd < 0) {
		test_caseFailed("Unable to open 'file:/bin'");
		return;
	}

	while((e = readdir(fd)));
	closedir(fd);

	test_caseSucceded();
}
