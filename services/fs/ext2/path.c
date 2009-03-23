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
#include "inodecache.h"
#include "request.h"

tInodeNo ext2_resolvePath(sExt2 *e,string path) {
	sCachedInode *cnode = NULL;
	tInodeNo res;
	s8 *p = path;
	u32 pos;

	if(*p != '/')
		return ERR_INVALID_PATH;

	cnode = ext2_icache_request(e,EXT2_ROOT_INO);
	if(cnode == NULL)
		return ERR_FS_READ_FAILED;

	/* skip '/' */
	p++;

	pos = strchri(p,'/');
	while(*p) {
		sDirEntry *eBak;
		sDirEntry *entry = (sDirEntry*)malloc(sizeof(u8) * BLOCK_SIZE(e));
		if(entry == NULL) {
			ext2_icache_release(e,cnode);
			return ERR_NOT_ENOUGH_MEM;
		}

		/* TODO a directory may have more blocks */
		eBak = entry;
		if(!ext2_readBlocks(e,(u8*)entry,cnode->inode.dBlocks[0],1)) {
			ext2_icache_release(e,cnode);
			return ERR_FS_READ_FAILED;
		}

		while(entry->inode != 0) {
			if(pos == entry->nameLen && strncmp((cstring)(entry + 1),p,pos) == 0) {
				p += pos;
				ext2_icache_release(e,cnode);
				cnode = ext2_icache_request(e,entry->inode);
				if(cnode == NULL) {
					free(eBak);
					return ERR_FS_READ_FAILED;
				}

				/* skip slashes */
				while(*p == '/')
					p++;
				/* "/" at the end is optional */
				if(!*p)
					break;

				/* move to childs of this node */
				pos = strchri(p,'/');
				if((cnode->inode.mode & EXT2_S_IFDIR) == 0) {
					ext2_icache_release(e,cnode);
					free(eBak);
					return ERR_NO_DIRECTORY;
				}
				break;
			}

			/* to next dir-entry */
			entry = (sDirEntry*)((u8*)entry + entry->recLen);
		}

		/* no match? */
		if(entry->inode == 0) {
			free(eBak);
			ext2_icache_release(e,cnode);
			return ERR_PATH_NOT_FOUND;
		}

		free(eBak);
	}

	res = cnode->inodeNo;
	ext2_icache_release(e,cnode);
	if(res != EXT2_BAD_INO)
		return res;
	return ERR_PATH_NOT_FOUND;
}
