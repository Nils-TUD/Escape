/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/heap.h>
#include <string.h>
#include <errors.h>

#include "ext2.h"
#include "path.h"
#include "inode.h"
#include "inodecache.h"
#include "request.h"
#include "file.h"

tInodeNo ext2_resolvePath(sExt2 *e,char *path) {
	sCachedInode *cnode = NULL;
	tInodeNo res;
	char *p = path;
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
		s32 rem = cnode->inode.size;
		sDirEntry *eBak;
		sDirEntry *entry;
		entry = (sDirEntry*)malloc(sizeof(u8) * cnode->inode.size);
		if(entry == NULL) {
			ext2_icache_release(e,cnode);
			return ERR_NOT_ENOUGH_MEM;
		}

		eBak = entry;
		if(ext2_readFile(e,cnode->inodeNo,entry,0,cnode->inode.size) < 0) {
			free(eBak);
			ext2_icache_release(e,cnode);
			return ERR_FS_READ_FAILED;
		}

		while(rem > 0 && entry->inode != 0) {
			if(pos == entry->nameLen && strncmp(entry->name,p,pos) == 0) {
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
			rem -= entry->recLen;
			entry = (sDirEntry*)((u8*)entry + entry->recLen);
		}

		/* no match? */
		if(rem <= 0 || entry->inode == 0) {
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
