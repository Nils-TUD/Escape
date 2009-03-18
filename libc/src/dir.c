/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/dir.h"
#include "../h/io.h"

/* the dir-entry in which we'll write */
typedef struct {
	sDirEntry header;
	s8 name[MAX_NAME_LEN + 1];
} __attribute__((packed)) sDirEntryRead;

static sDirEntryRead dirEntry;

s32 opendir(cstring path) {
	return open(path,IO_READ);
}

sDirEntry *readdir(tFD dir) {
	u32 len;
	if(read(dir,(u8*)&dirEntry,sizeof(sDirEntry)) > 0) {
		len = dirEntry.header.recLen - sizeof(sDirEntry);
		if(read(dir,(u8*)dirEntry.name,len) > 0) {
			dirEntry.name[dirEntry.header.nameLen] = '\0';
			return (sDirEntry*)&dirEntry;
		}
	}
	return NULL;
}

void closedir(tFD dir) {
	close(dir);
}
