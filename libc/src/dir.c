/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/dir.h"
#include "../h/io.h"

s32 opendir(cstring path) {
	return open(path,IO_READ);
}

sDir *readdir(tFD dir) {
	static sDir dirEntry;
	s32 c;
	if((c = read(dir,(u8*)&dirEntry,sizeof(sDir))) > 0)
		return &dirEntry;
	return NULL;
}

void closedir(tFD dir) {
	close(dir);
}
