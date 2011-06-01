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
#include <esc/endian.h>
#include <stdlib.h>
#include <errors.h>
#include <string.h>
#include <assert.h>

#include "ext2.h"
#include "dir.h"
#include "file.h"
#include "link.h"
#include "inodecache.h"

int ext2_dir_create(sExt2 *e,sExt2CInode *dir,const char *name) {
	sExt2CInode *cnode;
	tInodeNo ino;

	/* first create an inode and an entry in the directory */
	int res = ext2_file_create(e,dir,name,&ino,true);
	if(res < 0)
		return res;

	/* get created inode */
	cnode = ext2_icache_request(e,ino,IMODE_WRITE);
	vassert(cnode != NULL,"Unable to load inode %d\n",ino);

	/* create '.' and '..' */
	if((res = ext2_link_create(e,cnode,cnode,".")) < 0) {
		ext2_file_delete(e,cnode);
		ext2_icache_release(cnode);
		return res;
	}
	if((res = ext2_link_create(e,cnode,dir,"..")) < 0) {
		ext2_link_delete(e,dir,cnode,".",true);
		ext2_file_delete(e,cnode);
		ext2_icache_release(cnode);
		return res;
	}

	/* just to be sure */
	ext2_icache_markDirty(cnode);
	ext2_icache_release(cnode);
	return 0;
}

tInodeNo ext2_dir_find(sExt2 *e,sExt2CInode *dir,const char *name,size_t nameLen) {
	tInodeNo ino;
	size_t size = le32tocpu(dir->inode.size);
	int res;
	sExt2DirEntry *buffer = (sExt2DirEntry*)malloc(size);
	if(buffer == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* read the directory */
	if((res = ext2_file_readIno(e,dir,buffer,0,size)) < 0) {
		free(buffer);
		return res;
	}

	ino = ext2_dir_findIn(buffer,size,name,nameLen);
	free(buffer);
	return ino;
}

tInodeNo ext2_dir_findIn(sExt2DirEntry *buffer,size_t bufSize,const char *name,size_t nameLen) {
	ssize_t rem = bufSize;
	sExt2DirEntry *entry = buffer;

	/* search the directory-entries */
	while(rem > 0 && le32tocpu(entry->inode) != 0) {
		/* found a match? */
		if(nameLen == le16tocpu(entry->nameLen) && strncmp(entry->name,name,nameLen) == 0) {
			tInodeNo ino = le32tocpu(entry->inode);
			return ino;
		}

		/* to next dir-entry */
		rem -= le16tocpu(entry->recLen);
		entry = (sExt2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	return ERR_PATH_NOT_FOUND;
}

int ext2_dir_delete(sExt2 *e,sExt2CInode *dir,const char *name) {
	tInodeNo ino;
	size_t size = le32tocpu(dir->inode.size);
	int res;
	sExt2CInode *delIno;
	sExt2DirEntry *entry,*buffer;

	/* find the entry in the given directory */
	ino = ext2_dir_find(e,dir,name,strlen(name));
	if(ino < 0)
		return ino;
	/* get inode of directory to delete */
	delIno = ext2_icache_request(e,ino,IMODE_WRITE);
	if(delIno == NULL)
		return ERR_INO_REQ_FAILED;

	/* read the directory */
	buffer = (sExt2DirEntry*)malloc(size);
	if(buffer == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto error;
	}
	if((res = ext2_file_readIno(e,delIno,buffer,0,size)) < 0)
		goto error;

	/* search for other entries than '.' and '..' */
	entry = buffer;
	while(size > 0 && entry->inode != 0) {
		uint16_t namelen = le16tocpu(entry->nameLen);
		/* found a match? */
		if(namelen != 1 && namelen != 2 &&
				strncmp(entry->name,".",namelen) != 0 &&
				strncmp(entry->name,"..",namelen) != 0) {
			res = ERR_DIR_NOT_EMPTY;
			goto error;
		}

		/* to next dir-entry */
		size -= le16tocpu(entry->recLen);
		entry = (sExt2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	free(buffer);
	buffer = NULL;

	/* ok, directory is empty, so remove '.' and '..' */
	if((res = ext2_link_delete(e,dir,delIno,".",true)) < 0 ||
			(res = ext2_link_delete(e,dir,delIno,"..",true)) < 0)
		goto error;
	/* first release the del-inode, since ext2_link_delete will request it again */
	ext2_icache_release(delIno);
	/* now remove directory from parent, which will delete it because of no more references */
	res = ext2_link_delete(e,NULL,dir,name,true);
	free(buffer);
	return res;

error:
	free(buffer);
	ext2_icache_release(delIno);
	return res;
}
