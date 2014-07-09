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
#include <esc/io.h>
#include <esc/endian.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "ext2.h"
#include "path.h"
#include "inode.h"
#include "inodecache.h"
#include "rw.h"
#include "file.h"
#include "dir.h"

inode_t Ext2Path::resolve(Ext2FileSystem *e,FSUser *u,const char *path,uint flags) {
	Ext2CInode *cnode = NULL;
	inode_t res;
	const char *p = path;
	int err;
	ssize_t pos;

	while(*p == '/')
		p++;

	cnode = e->inodeCache.request(EXT2_ROOT_INO,IMODE_READ);
	if(cnode == NULL)
		return -ENOBUFS;

	pos = strchri(p,'/');
	while(*p) {
		/* we need execute-permission to access the directory */
		if((err = e->hasPermission(cnode,u,MODE_EXEC)) < 0) {
			e->inodeCache.release(cnode);
			return err;
		}

		res = Ext2Dir::find(e,cnode,p,pos);
		if(res >= 0) {
			p += pos;
			e->inodeCache.release(cnode);
			cnode = e->inodeCache.request(res,IMODE_READ);
			if(cnode == NULL)
				return -ENOBUFS;

			/* skip slashes */
			while(*p == '/')
				p++;
			/* "/" at the end is optional */
			if(!*p)
				break;

			/* move to childs of this node */
			pos = strchri(p,'/');
			if((le16tocpu(cnode->inode.mode) & EXT2_S_IFDIR) == 0) {
				e->inodeCache.release(cnode);
				return -ENOTDIR;
			}
		}
		/* no match? */
		else {
			char *slash = strchr(p,'/');
			/* should we create a new file? */
			if((slash == NULL || *(slash + 1) == '\0') && (flags & O_CREAT)) {
				/* rerequest inode for writing */
				res = cnode->inodeNo;
				e->inodeCache.release(cnode);
				cnode = e->inodeCache.request(res,IMODE_WRITE);
				if(cnode == NULL)
					return -ENOENT;
				/* ensure that there is no '/' in the name */
				if(slash)
					*slash = '\0';
				err = Ext2File::create(e,u,cnode,p,&res,false);
				e->inodeCache.release(cnode);
				if(err < 0)
					return err;
				return res;
			}
			else {
				e->inodeCache.release(cnode);
				return -ENOENT;
			}
		}
	}

	res = cnode->inodeNo;
	e->inodeCache.release(cnode);
	if(res != EXT2_BAD_INO) {
		if(flags & O_EXCL)
			return -EEXIST;
		return res;
	}
	return -ENOENT;
}
