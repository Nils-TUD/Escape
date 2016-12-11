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

#include <esc/util.h>
#include <fs/blockcache.h>
#include <sys/common.h>
#include <sys/io.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "iso9660.h"
#include "rw.h"

using namespace fs;

ino_t ISO9660Dir::resolve(ISO9660FileSystem *h,A_UNUSED User *u,const char *path,uint flags) {
	const char *p = path;
	while(*p == '/')
		p++;

	size_t extLoc = h->primary.data.primary.rootDir.extentLoc.littleEndian;
	size_t extSize = h->primary.data.primary.rootDir.extentSize.littleEndian;
	ino_t res = h->rootDirId();

	ssize_t pos = strchri(p,'/');
	while(*p) {
		const ISODirEntry *e;
		res = find(h,extLoc,extSize,p,pos,&e);
		if(res == -ENOBUFS)
			return res;
		if(res < 0) {
			if(flags & O_CREAT)
				return -EROFS;
			return -ENOENT;
		}

		p += pos;

		/* skip slashes */
		while(*p == '/')
			p++;
		/* "/" at the end is optional */
		if(!*p)
			break;

		/* move to childs of this node */
		pos = strchri(p,'/');
		if((e->flags & ISO_FILEFL_DIR) == 0)
			return -ENOTDIR;
		extLoc = e->extentLoc.littleEndian;
		extSize = e->extentSize.littleEndian;
	}
	return res;
}

ino_t ISO9660Dir::find(ISO9660FileSystem *h,size_t extLoc,size_t extSize,const char *name,
		size_t nameLen,const ISODirEntry **entry) {
	size_t blockSize = h->blockSize();
	CBlock *blk = h->blockCache.request(extLoc,BlockCache::READ);
	if(blk == NULL)
		return -ENOBUFS;

	size_t i = 0;
	size_t off = 0;
	const ISODirEntry *e = (const ISODirEntry*)blk->buffer;
	while((uintptr_t)e < (uintptr_t)blk->buffer + blockSize) {
		/* continue with next block? */
		if(e->length == 0) {
			off += blockSize;
			if(off >= extSize)
				break;

			h->blockCache.release(blk);
			blk = h->blockCache.request(extLoc + ++i,BlockCache::READ);
			if(blk == NULL)
				return -ENOBUFS;

			e = (const ISODirEntry*)blk->buffer;
			continue;
		}

		if(match(name,e->name,nameLen,e->nameLen)) {
			h->blockCache.release(blk);
			ino_t ino = getIno(extLoc + i,blockSize,(uintptr_t)e - (uintptr_t)blk->buffer);
			*entry = e;
			return ino;
		}

		e = (const ISODirEntry*)((uintptr_t)e + e->length);
	}

	h->blockCache.release(blk);
	return -ENOENT;
}

bool ISO9660Dir::match(const char *user,const char *disk,size_t userLen,size_t diskLen) {
	size_t rpos;
	if(*disk == ISO_FILENAME_THIS)
		return userLen == 1 && strcmp(user,".") == 0;
	if(*disk == ISO_FILENAME_PARENT)
		return userLen == 2 && strcmp(user,"..") == 0;
	/* don't compare volume sequence no */
	rpos = esc::Util::min(diskLen,(size_t)strchri(disk,';'));
	if(disk[rpos] != ';')
		return userLen == diskLen && strncasecmp(disk,user,userLen) == 0;
	return userLen == rpos && strncasecmp(disk,user,userLen) == 0;
}
