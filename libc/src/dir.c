/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include "../h/common.h"
#include "../h/dir.h"
#include "../h/io.h"

#define MAX_NAME_LEN 255

/* a VFS-node */
typedef struct {
	tVFSNodeNo nodeNo;
	u8 nameLen;
} sVFSNode;

s32 opendir(cstring path) {
	return open(path,IO_READ);
}

sDir *readdir(tFD dir) {
	static s8 nameBuf[MAX_NAME_LEN];
	static sDir dirEntry;
	sVFSNode node;
	s32 c;
	if((c = read(dir,(u8*)&node,sizeof(sVFSNode))) > 0) {
		if((c = read(dir,(u8*)nameBuf,node.nameLen)) > 0) {
			dirEntry.name = nameBuf;
			dirEntry.nodeNo = node.nodeNo;
			return &dirEntry;
		}
	}
	return NULL;
}

void closedir(tFD dir) {
	close(dir);
}
