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
#include <string.h>
#include <errors.h>
#include "iso9660.h"
#include "rw.h"
#include "dir.h"
#include "../mount.h"

/**
 * Checks wether <user> matches <disk>
 */
static bool iso_dir_match(const char *user,const char *disk);

tInodeNo iso_dir_resolve(sISO9660 *h,char *path,u8 flags,tDevNo *dev,bool resLastMnt) {
	u32 extLoc,extSize;
	tDevNo mntDev;
	char *p = path;
	u32 pos;
	tInodeNo res;

	while(*p == '/')
		p++;

	extLoc = h->primary.data.primary.rootDir.extentLoc.littleEndian;
	extSize = h->primary.data.primary.rootDir.extentSize.littleEndian;
	res = ISO_ROOT_DIR_ID(h);

	pos = strchri(p,'/');
	while(*p) {
		sISODirEntry *e;
		sISODirEntry *content = (sISODirEntry*)malloc(extSize);
		if(content == NULL)
			return ERR_NOT_ENOUGH_MEM;
		if(!iso_rw_readBlocks(h,content,extLoc,extSize / ISO_BLK_SIZE(h))) {
			free(content);
			return ERR_PATH_NOT_FOUND;
		}

		e = content;
		while((u8*)e < (u8*)content + extSize) {
			if(e->length == 0)
				break;
			if(iso_dir_match(p,e->name)) {
				p += pos;

				/* skip slashes */
				while(*p == '/')
					p++;
				/* "/" at the end is optional */
				if(!resLastMnt && !*p)
					break;

				/* is it a mount-point? */
				mntDev = mount_getByLoc(*dev,extLoc);
				if(mntDev >= 0) {
					sFSInst *inst = mount_get(mntDev);
					*dev = mntDev;
					free(content);
					return inst->fs->resPath(inst->handle,p,flags,dev,resLastMnt);
				}
				if(!*p)
					break;

				/* move to childs of this node */
				pos = strchri(p,'/');
				if((e->flags & ISO_FILEFL_DIR) == 0) {
					free(content);
					return ERR_NO_DIRECTORY;
				}
				extLoc = e->extentLoc.littleEndian;
				extSize = e->extentSize.littleEndian;
				break;
			}
			e = (sISODirEntry*)((u8*)e + e->length);
		}
		/* no match? */
		if(e->length == 0) {
			free(content);
			return ERR_PATH_NOT_FOUND;
		}
		res = (extLoc * ISO_BLK_SIZE(h)) + ((u8*)e - (u8*)content);
		free(content);
	}
	return res;
}

static bool iso_dir_match(const char *user,const char *disk) {
	if(*disk == ISO_FILENAME_THIS)
		return strcmp(user,".") == 0;
	if(*disk == ISO_FILENAME_PARENT)
		return strcmp(user,"..") == 0;
	/* don't compare volume sequence no */
	u32 rpos = strchri(disk,';');
	return rpos > 0 && strncmp(disk,user,rpos) == 0;
}
