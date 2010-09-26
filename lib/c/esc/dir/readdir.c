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
#include <esc/dir.h>
#include <stdio.h>

#define DIRE_SIZE	(sizeof(sDirEntry) - (MAX_NAME_LEN + 1))

DIR *opendir(const char *path) {
	return (DIR*)fopen(path,"r");
}

bool readdir(DIR *dir,sDirEntry *e) {
	if(fread(e,1,DIRE_SIZE,dir) > 0) {
		u32 len = e->nameLen;
		/* ensure that the name is short enough */
		if(len >= MAX_NAME_LEN)
			return false;

		/* now read the name */
		if(fread(e->name,1,len,dir) > 0) {
			/* if the record is longer, we have to skip the stuff until the next record */
			if(e->recLen - DIRE_SIZE > len) {
				len = (e->recLen - DIRE_SIZE - len);
				if(fseek(dir,len,SEEK_CUR) < 0)
					return false;
			}

			/* ensure that it is null-terminated */
			e->name[e->nameLen] = '\0';
			return true;
		}
	}

	return false;
}

void closedir(DIR *dir) {
	fclose(dir);
}
