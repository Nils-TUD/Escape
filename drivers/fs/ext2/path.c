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
#include <esc/io.h>
#include <string.h>
#include <errors.h>
#include <stdlib.h>

#include "ext2.h"
#include "path.h"
#include "inode.h"
#include "inodecache.h"
#include "rw.h"
#include "file.h"
#include "dir.h"
#include "../mount.h"

tInodeNo ext2_path_resolve(sExt2 *e,const char *path,u8 flags,tDevNo *dev,bool resLastMnt) {
	sExt2CInode *cnode = NULL;
	tInodeNo res;
	tDevNo mntDev;
	const char *p = path;
	s32 err;
	u32 pos;

	while(*p == '/')
		p++;

	cnode = ext2_icache_request(e,EXT2_ROOT_INO,IMODE_READ);
	if(cnode == NULL)
		return ERR_INO_REQ_FAILED;

	pos = strchri(p,'/');
	while(*p) {
		res = ext2_dir_find(e,cnode,p,pos);
		if(res >= 0) {
			p += pos;
			ext2_icache_release(cnode);
			cnode = ext2_icache_request(e,res,IMODE_READ);
			if(cnode == NULL)
				return ERR_INO_REQ_FAILED;

			/* skip slashes */
			while(*p == '/')
				p++;
			/* "/" at the end is optional */
			if(!resLastMnt && !*p)
				break;

			/* is it a mount-point? */
			mntDev = mount_getByLoc(*dev,res);
			if(mntDev >= 0) {
				sFSInst *inst = mount_get(mntDev);
				*dev = mntDev;
				ext2_icache_release(cnode);
				return inst->fs->resPath(inst->handle,p,flags,dev,resLastMnt);
			}
			if(!*p)
				break;

			/* move to childs of this node */
			pos = strchri(p,'/');
			if((cnode->inode.mode & EXT2_S_IFDIR) == 0) {
				ext2_icache_release(cnode);
				return ERR_NO_DIRECTORY;
			}
		}
		/* no match? */
		else {
			char *slash = strchr(p,'/');
			/* should we create a new file? */
			if((slash == NULL || *(slash + 1) == '\0') && (flags & IO_CREATE)) {
				/* rerequest inode for writing */
				res = cnode->inodeNo;
				ext2_icache_release(cnode);
				cnode = ext2_icache_request(e,res,IMODE_WRITE);
				if(cnode == NULL)
					return ERR_PATH_NOT_FOUND;
				/* ensure that there is no '/' in the name */
				if(slash)
					*slash = '\0';
				err = ext2_file_create(e,cnode,p,&res,false);
				ext2_icache_release(cnode);
				if(err < 0)
					return err;
				return res;
			}
			else {
				ext2_icache_release(cnode);
				return ERR_PATH_NOT_FOUND;
			}
		}
	}

	res = cnode->inodeNo;
	ext2_icache_release(cnode);
	if(res != EXT2_BAD_INO)
		return res;
	return ERR_PATH_NOT_FOUND;
}
