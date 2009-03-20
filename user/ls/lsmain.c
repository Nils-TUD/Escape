/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <dir.h>
#include <io.h>

s32 main(u32 argc,s8 *argv[]) {
	tFD dd;
	sDirEntry *entry;

	if(argc != 2) {
		printf("Usage: ls <dir>\n");
		return 1;
	}

	if((dd = opendir(argv[1])) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			printf("% 4d %s\n",entry->nodeNo,entry->name);
		}
		closedir(dd);
	}
	else {
		printf("Unable to open '%s'\n",argv[1]);
		return 1;
	}

	return 0;
}
