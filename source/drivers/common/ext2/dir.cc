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

#include <esc/common.h>
#include <esc/endian.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "ext2.h"
#include "dir.h"
#include "file.h"
#include "link.h"
#include "inodecache.h"

int Ext2Dir::create(Ext2FileSystem *e,FSUser *u,Ext2CInode *dir,const char *name) {
	Ext2CInode *cnode;
	ino_t ino;

	/* first create an inode and an entry in the directory */
	int res = Ext2File::create(e,u,dir,name,&ino,true);
	if(res < 0)
		return res;

	/* get created inode */
	cnode = e->inodeCache.request(ino,IMODE_WRITE);
	vassert(cnode != NULL,"Unable to load inode %d\n",ino);

	/* create '.' and '..' */
	if((res = Ext2Link::create(e,u,cnode,cnode,".")) < 0) {
		Ext2File::remove(e,cnode);
		e->inodeCache.release(cnode);
		return res;
	}
	if((res = Ext2Link::create(e,u,cnode,dir,"..")) < 0) {
		Ext2Link::remove(e,u,dir,cnode,".",true);
		Ext2File::remove(e,cnode);
		e->inodeCache.release(cnode);
		return res;
	}

	/* just to be sure */
	e->inodeCache.markDirty(cnode);
	e->inodeCache.release(cnode);
	return 0;
}

ino_t Ext2Dir::find(Ext2FileSystem *e,Ext2CInode *dir,const char *name,size_t nameLen) {
	ino_t ino;
	size_t size = le32tocpu(dir->inode.size);
	int res;
	Ext2DirEntry *buffer = (Ext2DirEntry*)malloc(size);
	if(buffer == NULL)
		return -ENOMEM;

	/* read the directory */
	if((res = Ext2File::readIno(e,dir,buffer,0,size)) < 0) {
		free(buffer);
		return res;
	}

	ino = findIn(buffer,size,name,nameLen);
	free(buffer);
	return ino;
}

ino_t Ext2Dir::findIn(Ext2DirEntry *buffer,size_t bufSize,const char *name,size_t nameLen) {
	ssize_t rem = bufSize;
	Ext2DirEntry *entry = buffer;

	/* search the directory-entries */
	while(rem > 0 && le32tocpu(entry->inode) != 0) {
		/* found a match? */
		if(nameLen == le16tocpu(entry->nameLen) && strncmp(entry->name,name,nameLen) == 0) {
			ino_t ino = le32tocpu(entry->inode);
			return ino;
		}

		/* to next dir-entry */
		rem -= le16tocpu(entry->recLen);
		entry = (Ext2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	return -ENOENT;
}

int Ext2Dir::remove(Ext2FileSystem *e,FSUser *u,Ext2CInode *dir,const char *name) {
	ino_t ino;
	size_t size = le32tocpu(dir->inode.size);
	int res;
	Ext2CInode *delIno;
	Ext2DirEntry *entry,*buffer;

	/* we need write-permission to delete */
	if((res = e->hasPermission(dir,u,MODE_WRITE)) < 0)
		return res;

	/* find the entry in the given directory */
	ino = Ext2Dir::find(e,dir,name,strlen(name));
	if(ino < 0)
		return ino;
	/* get inode of directory to delete */
	delIno = e->inodeCache.request(ino,IMODE_WRITE);
	if(delIno == NULL)
		return -ENOBUFS;

	/* read the directory */
	buffer = (Ext2DirEntry*)malloc(size);
	if(buffer == NULL) {
		res = -ENOMEM;
		goto error;
	}
	if((res = Ext2File::readIno(e,delIno,buffer,0,size)) < 0)
		goto error;

	/* search for other entries than '.' and '..' */
	entry = buffer;
	while(size > 0 && entry->inode != 0) {
		uint16_t namelen = le16tocpu(entry->nameLen);
		/* found a match? */
		if(namelen != 1 && namelen != 2 &&
				strncmp(entry->name,".",namelen) != 0 &&
				strncmp(entry->name,"..",namelen) != 0) {
			res = -ENOTEMPTY;
			goto error;
		}

		/* to next dir-entry */
		size -= le16tocpu(entry->recLen);
		entry = (Ext2DirEntry*)((uintptr_t)entry + le16tocpu(entry->recLen));
	}
	free(buffer);
	buffer = NULL;

	/* ok, directory is empty, so remove '.' and '..' */
	if((res = Ext2Link::remove(e,u,dir,delIno,".",true)) < 0 ||
			(res = Ext2Link::remove(e,u,dir,delIno,"..",true)) < 0)
		goto error;
	/* first release the del-inode, since Ext2Link::delete will request it again */
	e->inodeCache.release(delIno);
	/* now remove directory from parent, which will delete it because of no more references */
	res = Ext2Link::remove(e,u,NULL,dir,name,true);
	free(buffer);
	return res;

error:
	free(buffer);
	e->inodeCache.release(delIno);
	return res;
}
