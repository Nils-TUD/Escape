/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <env.h>

s32 main(u32 argc,s8 **argv) {
	s8 *val;

	val = getenv("CWD");
	printf("CWD=%s\n",val ? val : "");

	val = getenv("PATH");
	printf("PATH=%s\n",val ? val : "");

	setenv("ABC","das ist mein wert");
	val = getenv("ABC");
	printf("ABC=%s\n",val ? val : "");

	setenv("PATH","file:/services");
	val = getenv("PATH");
	printf("PATH=%s\n",val ? val : "");
	return 0;
}
