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
#include <esc/fsinterface.h>
#include <esc/endian.h>
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
static size_t ext2_link_getDirESize(size_t namelen);

int ext2_link_create(sExt2 *e,sFSUser *u,sExt2CInode *dir,sExt2CInode *cnode,const char *name) {
	uint8_t *buf;
	sExt2DirEntry *dire;
	size_t len = strlen(name);
	size_t tlen = ext2_link_getDirESize(len);
	size_t recLen = 0;
	int res;
	int32_t dirSize = le32tocpu(dir->inode.size);

	/* we need write-permission to create dir-entries */
	if((res = ext2_hasPermission(dir,u,MODE_WRITE)) < 0)
		return res;

	/* TODO we don't have to read the whole directory at once */

	/* read directory-entries */
	buf = malloc(dirSize + tlen);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if((res = ext2_file_readIno(e,dir,buf,0,dirSize)) != dirSize) {
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
	while((uint8_t*)dire < buf + dirSize) {
		/* does our entry fit? */
		size_t elen = ext2_link_getDirESize(le16tocpu(dire->nameLen));
		uint16_t orgRecLen = le16tocpu(dire->recLen);
		if(elen < orgRecLen && orgRecLen - elen >= tlen) {
			recLen = orgRecLen - elen;
			/* adjust old entry */
			dire->recLen = cputole16(orgRecLen - recLen);
			dire = (sExt2DirEntry*)((uintptr_t)dire + elen);
			break;
		}
		dire = (sExt2DirEntry*)((uintptr_t)dire + orgRecLen);
	}
	/* nothing found yet? so store it on the next block */
	if(recLen == 0) {
		dire = (sExt2DirEntry*)(buf + dirSize);
		recLen = EXT2_BLK_SIZE(e);
		dirSize += recLen;
	}

	/* build entry */
	dire->inode = cputole32(cnode->inodeNo);
	dire->nameLen = cputole16(len);
	dire->recLen = cputole16(recLen);
	memcpy(dire->name,name,len);

	/* write it back */
	if((res = ext2_file_writeIno(e,dir,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}
	free(buf);

	/* increase link-count */
	cnode->inode.linkCount = cputole16(le16tocpu(cnode->inode.linkCount) + 1);
	ext2_icache_markDirty(cnode);
	return 0;
}

int ext2_link_delete(sExt2 *e,sFSUser *u,sExt2CInode *pdir,sExt2CInode *dir,const char *name,
		bool delDir) {
	uint8_t *buf;
	size_t nameLen;
	inode_t ino = -1;
	sExt2DirEntry *dire,*prev;
	int res;
	int32_t dirSize = le32tocpu(dir->inode.size);
	sExt2CInode *cnode;

	/* we need write-permission to delete dir-entries */
	if((res = ext2_hasPermission(dir,u,MODE_WRITE)) < 0)
		return res;

	/* read directory-entries */
	buf = malloc(dirSize);
	if(buf == NULL)
		return ERR_NOT_ENOUGH_MEM;
	if((res = ext2_file_readIno(e,dir,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}

	/* search our entry */
	nameLen = strlen(name);
	prev = NULL;
	dire = (sExt2DirEntry*)buf;
	while((uint8_t*)dire < buf + dirSize) {
		if(nameLen == le16tocpu(dire->nameLen) && strncmp(dire->name,name,nameLen) == 0) {
			ino = le32tocpu(dire->inode);
			if(pdir && ino == pdir->inodeNo)
				cnode = pdir;
			else if(ino == dir->inodeNo)
				cnode = dir;
			else {
				cnode = ext2_icache_request(e,ino,IMODE_WRITE);
				if(cnode == NULL) {
					free(buf);
					return ERR_INO_REQ_FAILED;
				}
			}
			if(!delDir && S_ISDIR(le16tocpu(cnode->inode.mode))) {
				if(cnode != pdir && cnode != dir)
					ext2_icache_release(cnode);
				free(buf);
				return ERR_IS_DIR;
			}
			/* if we have a previous one, simply increase its length */
			if(prev != NULL)
				prev->recLen = cputole16(le16tocpu(prev->recLen) + le16tocpu(dire->recLen));
			/* otherwise make an empty entry */
			else
				dire->inode = cputole32(0);
			break;
		}

		/* to next */
		prev = dire;
		dire = (sExt2DirEntry*)((uintptr_t)dire + le16tocpu(dire->recLen));
	}

	/* no match? */
	if(ino == -1) {
		free(buf);
		return ERR_PATH_NOT_FOUND;
	}

	/* write it back */
	if((res = ext2_file_writeIno(e,dir,buf,0,dirSize)) != dirSize) {
		if(cnode && cnode != pdir && cnode != dir)
			ext2_icache_release(cnode);
		free(buf);
		return res;
	}
	free(buf);

	/* update inode */
	if(cnode != NULL) {
		uint16_t linkCount;
		/* decrease link-count */
		ext2_icache_markDirty(cnode);
		linkCount = le16tocpu(cnode->inode.linkCount) - 1;
		cnode->inode.linkCount = cputole16(linkCount);
		if(linkCount == 0) {
			/* delete the file if there are no references anymore */
			if((res = ext2_file_delete(e,cnode)) < 0) {
				if(cnode != pdir && cnode != dir)
					ext2_icache_release(cnode);
				return res;
			}
		}
		if(cnode != pdir && cnode != dir)
			ext2_icache_release(cnode);
	}
	return 0;
}

static size_t ext2_link_getDirESize(size_t namelen) {
	size_t tlen = namelen + sizeof(sExt2DirEntry);
	if((tlen % EXT2_DIRENTRY_PAD) != 0)
		tlen += EXT2_DIRENTRY_PAD - (tlen % EXT2_DIRENTRY_PAD);
	return tlen;
}
