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
#include <assert.h>
#include "ext2.h"
#include "dir.h"
#include "file.h"
#include "link.h"
#include "inodecache.h"

s32 ext2_dir_create(sExt2 *e,sCachedInode *dir,const char *name) {
	sCachedInode *cnode;
	tInodeNo ino;

	/* first create an inode and an entry in the directory */
	s32 res = ext2_file_create(e,dir,name,&ino,true);
	if(res < 0)
		return res;

	/* get created inode */
	cnode = ext2_icache_request(e,ino);
	vassert(cnode != NULL,"Unable to load inode %d\n",ino);

	/* create '.' and '..' */
	if((res = ext2_link_create(e,cnode,cnode,".")) < 0) {
		ext2_file_delete(e,cnode);
		ext2_icache_release(e,cnode);
		return res;
	}
	if((res = ext2_link_create(e,cnode,dir,"..")) < 0) {
		ext2_link_delete(e,cnode,".");
		ext2_file_delete(e,cnode);
		ext2_icache_release(e,cnode);
		return res;
	}

	/* just to be sure */
	cnode->dirty = true;
	ext2_icache_release(e,cnode);
	return 0;
}

tInodeNo ext2_dir_find(sExt2 *e,sCachedInode *dir,const char *name,u32 nameLen) {
	tInodeNo ino;
	s32 size = dir->inode.size;
	sExt2DirEntry *buffer = (sExt2DirEntry*)malloc(sizeof(u8) * size);
	if(buffer == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* read the directory */
	if(ext2_file_read(e,dir->inodeNo,buffer,0,size) < 0) {
		free(buffer);
		return ERR_FS_READ_FAILED;
	}

	ino = ext2_dir_findIn(buffer,size,name,nameLen);
	free(buffer);
	return ino;
}

tInodeNo ext2_dir_findIn(sExt2DirEntry *buffer,u32 bufSize,const char *name,u32 nameLen) {
	s32 rem = bufSize;
	sExt2DirEntry *entry = buffer;

	/* search the directory-entries */
	while(rem > 0 && entry->inode != 0) {
		/* found a match? */
		if(nameLen == entry->nameLen && strncmp(entry->name,name,nameLen) == 0) {
			tInodeNo ino = entry->inode;
			return ino;
		}

		/* to next dir-entry */
		rem -= entry->recLen;
		entry = (sExt2DirEntry*)((u8*)entry + entry->recLen);
	}
	return ERR_FS_NOT_FOUND;
}

s32 ext2_dir_delete(sExt2 *e,sCachedInode *dir,const char *name) {
	tInodeNo ino;
	s32 res,size = dir->inode.size;
	sCachedInode *delIno;
	sExt2DirEntry *entry,*buffer;

	/* find the entry in the given directory */
	ino = ext2_dir_find(e,dir,name,strlen(name));
	if(ino < 0)
		return ino;
	/* get inode of directory to delete */
	delIno = ext2_icache_request(e,ino);
	if(delIno == NULL)
		return ERR_FS_INODE_NOT_FOUND;

	/* read the directory */
	buffer = (sExt2DirEntry*)malloc(sizeof(u8) * size);
	if(buffer == NULL) {
		res = ERR_NOT_ENOUGH_MEM;
		goto error;
	}
	if(ext2_file_read(e,ino,buffer,0,size) < 0) {
		res = ERR_FS_READ_FAILED;
		goto error;
	}

	/* search for other entries than '.' and '..' */
	entry = buffer;
	while(size > 0 && entry->inode != 0) {
		/* found a match? */
		if(entry->nameLen != 1 && entry->nameLen != 2 &&
				strncmp(entry->name,".",entry->nameLen) != 0 &&
				strncmp(entry->name,"..",entry->nameLen) != 0) {
			res = ERR_FS_DIR_NOT_EMPTY;
			goto error;
		}

		/* to next dir-entry */
		size -= entry->recLen;
		entry = (sExt2DirEntry*)((u8*)entry + entry->recLen);
	}
	free(buffer);
	buffer = NULL;

	/* ok, diretory is empty, so remove '.' and '..' */
	if((res = ext2_link_delete(e,delIno,".")) < 0 || (res = ext2_link_delete(e,delIno,"..")) < 0)
		goto error;
	/* now remove directory from parent, which will delete it because of no more references */
	res = ext2_link_delete(e,dir,name);

error:
	free(buffer);
	ext2_icache_release(e,delIno);
	return res;
}
