/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <sys/endian.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "ext2.h"
#include "file.h"
#include "inodecache.h"
#include "link.h"

int Ext2Link::create(Ext2FileSystem *e,FSUser *u,Ext2CInode *dir,Ext2CInode *cnode,const char *name) {
	uint8_t *buf;
	Ext2DirEntry *dire;
	size_t len = strlen(name);
	size_t tlen = getDirESize(len);
	size_t recLen = 0;
	int res;
	int32_t dirSize = le32tocpu(dir->inode.size);

	/* we need write-permission to create dir-entries */
	if((res = e->hasPermission(dir,u,MODE_WRITE)) < 0)
		return res;

	/* TODO we don't have to read the whole directory at once */

	/* read directory-entries */
	buf = static_cast<uint8_t*>(malloc(dirSize + tlen));
	if(buf == NULL)
		return -ENOMEM;
	if((res = Ext2File::readIno(e,dir,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}

	/* check if the entry exists */
	if(Ext2Dir::findIn((Ext2DirEntry*)buf,dirSize,name,len) >= 0) {
		free(buf);
		return -EEXIST;
	}

	/* search for a place for our entry */
	dire = (Ext2DirEntry*)buf;
	while((uint8_t*)dire < buf + dirSize) {
		/* does our entry fit? */
		size_t elen = getDirESize(le16tocpu(dire->nameLen));
		uint16_t orgRecLen = le16tocpu(dire->recLen);
		if(elen < orgRecLen && orgRecLen - elen >= tlen) {
			recLen = orgRecLen - elen;
			/* adjust old entry */
			dire->recLen = cputole16(orgRecLen - recLen);
			dire = (Ext2DirEntry*)((uintptr_t)dire + elen);
			break;
		}
		dire = (Ext2DirEntry*)((uintptr_t)dire + orgRecLen);
	}
	/* nothing found yet? so store it on the next block */
	if(recLen == 0) {
		dire = (Ext2DirEntry*)(buf + dirSize);
		recLen = e->blockSize();
		dirSize += recLen;
	}

	/* build entry */
	dire->inode = cputole32(cnode->inodeNo);
	dire->nameLen = cputole16(len);
	dire->recLen = cputole16(recLen);
	memcpy(dire->name,name,len);

	/* write it back */
	if((res = Ext2File::writeIno(e,dir,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}
	free(buf);

	/* increase link-count */
	cnode->inode.linkCount = cputole16(le16tocpu(cnode->inode.linkCount) + 1);
	e->inodeCache.markDirty(cnode);
	return 0;
}

int Ext2Link::remove(Ext2FileSystem *e,FSUser *u,Ext2CInode *pdir,Ext2CInode *dir,const char *name,
		bool delDir) {
	uint8_t *buf;
	size_t nameLen;
	ino_t ino = -1;
	Ext2DirEntry *dire,*prev;
	int res;
	int32_t dirSize = le32tocpu(dir->inode.size);
	Ext2CInode *cnode;

	/* we need write-permission to delete dir-entries */
	if((res = e->hasPermission(dir,u,MODE_WRITE)) < 0)
		return res;

	/* read directory-entries */
	buf = static_cast<uint8_t*>(malloc(dirSize));
	if(buf == NULL)
		return -ENOMEM;
	if((res = Ext2File::readIno(e,dir,buf,0,dirSize)) != dirSize) {
		free(buf);
		return res;
	}

	/* search our entry */
	nameLen = strlen(name);
	prev = NULL;
	dire = (Ext2DirEntry*)buf;
	while((uint8_t*)dire < buf + dirSize) {
		if(nameLen == le16tocpu(dire->nameLen) && strncmp(dire->name,name,nameLen) == 0) {
			ino = le32tocpu(dire->inode);
			if(pdir && ino == pdir->inodeNo)
				cnode = pdir;
			else if(ino == dir->inodeNo)
				cnode = dir;
			else {
				cnode = e->inodeCache.request(ino,IMODE_WRITE);
				if(cnode == NULL) {
					free(buf);
					return -ENOBUFS;
				}
			}
			if(!delDir && S_ISDIR(le16tocpu(cnode->inode.mode))) {
				if(cnode != pdir && cnode != dir)
					e->inodeCache.release(cnode);
				free(buf);
				return -EISDIR;
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
		dire = (Ext2DirEntry*)((uintptr_t)dire + le16tocpu(dire->recLen));
	}

	/* no match? */
	if(ino == -1) {
		free(buf);
		return -ENOENT;
	}

	/* write it back */
	if((res = Ext2File::writeIno(e,dir,buf,0,dirSize)) != dirSize) {
		if(cnode && cnode != pdir && cnode != dir)
			e->inodeCache.release(cnode);
		free(buf);
		return res;
	}
	free(buf);

	/* update inode */
	if(cnode != NULL) {
		uint16_t linkCount;
		/* decrease link-count */
		e->inodeCache.markDirty(cnode);
		linkCount = le16tocpu(cnode->inode.linkCount) - 1;
		cnode->inode.linkCount = cputole16(linkCount);
		/* don't delete the file here if linkCount is 0. we'll do that later when the last reference
		 * is gone */
		if(cnode != pdir && cnode != dir)
			e->inodeCache.release(cnode);
	}
	return 0;
}

size_t Ext2Link::getDirESize(size_t namelen) {
	size_t tlen = namelen + sizeof(Ext2DirEntry);
	if((tlen % EXT2_DIRENTRY_PAD) != 0)
		tlen += EXT2_DIRENTRY_PAD - (tlen % EXT2_DIRENTRY_PAD);
	return tlen;
}
