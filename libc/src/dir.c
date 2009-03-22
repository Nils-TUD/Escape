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

	/* TODO a lot of improvement is needed here ;) */

	if(read(dir,(u8*)&dirEntry,sizeof(sDirEntry)) > 0) {
		len = dirEntry.header.nameLen;
		if(len >= MAX_NAME_LEN)
			return NULL;

		if(read(dir,(u8*)dirEntry.name,len) > 0) {
			/* if the record is longer, we have to read the remaining stuff to somewhere :( */
			/* TODO maybe we should introduce a read-to-null? (if buffer = NULL, throw data away?) */
			if(dirEntry.header.recLen - sizeof(sDirEntry) > len) {
				len = (dirEntry.header.recLen - sizeof(sDirEntry) - len);
				u8 *tmp = (u8*)malloc(len);
				if(tmp == NULL)
					return NULL;
				read(dir,tmp,len);
				free(tmp);
			}

			/* ensure that it is null-terminated */
			dirEntry.name[dirEntry.header.nameLen] = '\0';
			return (sDirEntry*)&dirEntry;
		}
	}

	return NULL;
}

void closedir(tFD dir) {
	close(dir);
}
