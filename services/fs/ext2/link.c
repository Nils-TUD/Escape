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
#include <errors.h>
#include <string.h>
#include "ext2.h"
#include "file.h"
#include "link.h"

/**
 * Calculates the total size of a dir-entry, including padding
 */
static u32 ext2_getDirEntrySize(u32 namelen);

s32 ext2_link(sExt2 *e,sCachedInode *dir,sCachedInode *cnode,const char *name) {
	u8 *buf;
	sDirEntry *dire;
	u32 len = strlen(name);
	u32 tlen = ext2_getDirEntrySize(len);
	u32 recLen = 0;
	s32 dirSize = dir->inode.size;

	/* TODO we don't have to read the whole directory at once */

	/* read directory-entries */
	buf = malloc(dirSize + tlen);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if(ext2_readFile(e,dir->inodeNo,buf,0,dirSize) != dirSize) {
		free(buf);
		return ERR_FS_READ_FAILED;
	}

	/* search for a place for our entry */
	dire = (sDirEntry*)buf;
	while((u8*)dire < buf + dirSize) {
		/* does our entry fit? */
		u32 elen = ext2_getDirEntrySize(dire->nameLen);
		if(elen < dire->recLen && dire->recLen - elen >= tlen) {
			recLen = dire->recLen - elen;
			/* adjust old entry */
			dire->recLen -= recLen;
			dire = (sDirEntry*)((u8*)dire + elen);
			break;
		}
		dire = (sDirEntry*)((u8*)dire + dire->recLen);
	}
	/* nothing found yet? so store it on the next block */
	if(recLen == 0) {
		dire = (sDirEntry*)(buf + dirSize);
		recLen = BLOCK_SIZE(e);
		dirSize += recLen;
	}

	/* build entry */
	dire->inode = cnode->inodeNo;
	dire->nameLen = len;
	dire->recLen = recLen;
	memcpy(dire->name,name,len);

	/* write it back */
	if(ext2_writeFile(e,dir->inodeNo,buf,0,dirSize) != dirSize) {
		free(buf);
		return ERR_FS_WRITE_FAILED;
	}
	free(buf);

	/* increase link-count */
	cnode->inode.linkCount++;
	cnode->dirty = true;
	return 0;
}

s32 ext2_unlink(sExt2 *e,sCachedInode *dir,sCachedInode *cnode) {
	u8 *buf;
	sDirEntry *dire,*prev;
	s32 res,dirSize = dir->inode.size;

	/* read directory-entries */
	buf = malloc(dirSize);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if(ext2_readFile(e,dir->inodeNo,buf,0,dirSize) != dirSize) {
		free(buf);
		return ERR_FS_READ_FAILED;
	}

	/* search our entry */
	prev = NULL;
	dire = (sDirEntry*)buf;
	while((u8*)dire < buf + dirSize) {
		if(dire->inode == cnode->inodeNo) {
			/* if we have a previous one, simply increase its length */
			if(prev != NULL)
				prev->recLen += dire->recLen;
			/* otherwise make an empty entry */
			else {
				dire->inode = 0;
				dire->nameLen = 0;
			}
			break;
		}

		/* to next */
		prev = dire;
		dire = (sDirEntry*)((u8*)dire + dire->recLen);
	}

	/* write it back */
	if(ext2_writeFile(e,dir->inodeNo,buf,0,dirSize) != dirSize) {
		free(buf);
		return ERR_FS_WRITE_FAILED;
	}
	free(buf);

	/* decrease link-count */
	cnode->dirty = true;
	if(--cnode->inode.linkCount == 0) {
		/* delete the file if there are no references anymore */
		if((res = ext2_deleteFile(e,cnode->inodeNo)) < 0)
			return res;
	}
	return 0;
}

static u32 ext2_getDirEntrySize(u32 namelen) {
	u32 tlen = namelen + sizeof(sDirEntry);
	if((tlen % EXT2_DIRENTRY_PAD) != 0)
		tlen += EXT2_DIRENTRY_PAD - (tlen % EXT2_DIRENTRY_PAD);
	return tlen;
}
