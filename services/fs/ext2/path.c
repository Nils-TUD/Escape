/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <string.h>
#include <heap.h>
#include "ext2.h"
#include "path.h"
#include "inode.h"
#include "request.h"

sInode *ext2_resolvePath(sExt2 *e,string path) {
	sInode *inode = NULL;
	s8 *p = path;
	u32 pos;

	if(*p != '/') {
		/* TODO */
		return NULL;
	}

	inode = ext2_getInode(e,EXT2_ROOT_INO - 1);
	/* skip '/' */
	p++;

	pos = strchri(p,'/');
	while(*p) {
		sDirEntry *eBak;
		sDirEntry *entry = (sDirEntry*)malloc(sizeof(u8) * BLOCK_SIZE(e));
		if(entry == NULL)
			return NULL;

		/* TODO a directory may have more blocks */
		eBak = entry;
		ext2_readBlocks(e,(u8*)entry,inode->dBlocks[0],1);
		while(entry->inode != 0) {
			if(strncmp(entry + 1,p,pos) == 0) {
				p += pos;
				inode = ext2_getInode(e,entry->inode - 1);

				/* skip slashes */
				while(*p == '/')
					p++;
				/* "/" at the end is optional */
				if(!*p)
					break;

				/* move to childs of this node */
				pos = strchri(p,'/');
				if((inode->mode & EXT2_S_IFDIR) == 0) {
					free(entry);
					return NULL;
				}
				break;
			}

			/* to next dir-entry */
			entry = (u8*)entry + entry->recLen;
		}

		free(eBak);
	}

	return inode;
}
