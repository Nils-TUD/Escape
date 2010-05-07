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
#include <fsinterface.h>
#include <errors.h>
#include <string.h>
#include <stdlib.h>
#include "ext2.h"
#include "file.h"
#include "link.h"
#include "dir.h"
#include "inodecache.h"

/**
 * Calculates the total size of a dir-entry, including padding
 */
static u32 ext2_link_getDirESize(u32 namelen);

s32 ext2_link_create(sExt2 *e,sExt2CInode *dir,sExt2CInode *cnode,const char *name) {
	u8 *buf;
	sExt2DirEntry *dire;
	u32 len = strlen(name);
	u32 tlen = ext2_link_getDirESize(len);
	u32 recLen = 0;
	s32 res,dirSize = dir->inode.size;

	/* TODO we don't have to read the whole directory at once */

	/* read directory-entries */
	buf = malloc(dirSize + tlen);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if((res = ext2_file_read(e,dir->inodeNo,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}

	/* check if the entry exists */
	if(ext2_dir_findIn((sExt2DirEntry*)buf,dirSize,name,len) >= 0) {
		free(buf);
		return ERR_FILE_EXISTS;
	}

	/* search for a place for our entry */
	dire = (sExt2DirEntry*)buf;
	while((u8*)dire < buf + dirSize) {
		/* does our entry fit? */
		u32 elen = ext2_link_getDirESize(dire->nameLen);
		if(elen < dire->recLen && dire->recLen - elen >= tlen) {
			recLen = dire->recLen - elen;
			/* adjust old entry */
			dire->recLen -= recLen;
			dire = (sExt2DirEntry*)((u8*)dire + elen);
			break;
		}
		dire = (sExt2DirEntry*)((u8*)dire + dire->recLen);
	}
	/* nothing found yet? so store it on the next block */
	if(recLen == 0) {
		dire = (sExt2DirEntry*)(buf + dirSize);
		recLen = EXT2_BLK_SIZE(e);
		dirSize += recLen;
	}

	/* build entry */
	dire->inode = cnode->inodeNo;
	dire->nameLen = len;
	dire->recLen = recLen;
	memcpy(dire->name,name,len);

	/* write it back */
	if((res = ext2_file_write(e,dir->inodeNo,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}
	free(buf);

	/* increase link-count */
	cnode->inode.linkCount++;
	cnode->dirty = true;
	return 0;
}

s32 ext2_link_delete(sExt2 *e,sExt2CInode *dir,const char *name,bool delDir) {
	u8 *buf;
	u32 nameLen;
	tInodeNo ino = -1;
	sExt2DirEntry *dire,*prev;
	s32 res,dirSize = dir->inode.size;
	sExt2CInode *cnode;

	/* read directory-entries */
	buf = malloc(dirSize);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if((res = ext2_file_read(e,dir->inodeNo,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}

	/* search our entry */
	nameLen = strlen(name);
	prev = NULL;
	dire = (sExt2DirEntry*)buf;
	while((u8*)dire < buf + dirSize) {
		if(nameLen == dire->nameLen && strncmp(dire->name,name,nameLen) == 0) {
			ino = dire->inode;
			cnode = ext2_icache_request(e,ino);
			if(cnode == NULL) {
				free(buf);
				return ERR_INO_REQ_FAILED;
			}
			if(!delDir && MODE_IS_DIR(cnode->inode.mode)) {
				free(buf);
				return ERR_IS_DIR;
			}
			/* if we have a previous one, simply increase its length */
			if(prev != NULL)
				prev->recLen += dire->recLen;
			/* otherwise make an empty entry */
			else
				dire->inode = 0;
			break;
		}

		/* to next */
		prev = dire;
		dire = (sExt2DirEntry*)((u8*)dire + dire->recLen);
	}

	/* no match? */
	if(ino == -1) {
		free(buf);
		return ERR_PATH_NOT_FOUND;
	}

	/* write it back */
	if((res = ext2_file_write(e,dir->inodeNo,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}
	free(buf);

	/* update inode */
	if(cnode != NULL) {
		/* decrease link-count */
		cnode->dirty = true;
		if(--cnode->inode.linkCount == 0) {
			/* delete the file if there are no references anymore */
			if((res = ext2_file_delete(e,cnode)) < 0) {
				ext2_icache_release(e,cnode);
				return res;
			}
		}
		ext2_icache_release(e,cnode);
	}
	return 0;
}

static u32 ext2_link_getDirESize(u32 namelen) {
	u32 tlen = namelen + sizeof(sExt2DirEntry);
	if((tlen % EXT2_DIRENTRY_PAD) != 0)
		tlen += EXT2_DIRENTRY_PAD - (tlen % EXT2_DIRENTRY_PAD);
	return tlen;
}
