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
#include <dirent.h>
#include <stdio.h>

#define DIRE_SIZE	(sizeof(struct dirent) - (NAME_MAX + 1))

bool readdirto(DIR *dir,struct dirent *e) {
	if(fread(e,1,DIRE_SIZE,dir) > 0) {
		/* convert endianess */
		e->d_namelen = le16tocpu(e->d_namelen);
		e->d_reclen = le16tocpu(e->d_reclen);
		e->d_ino = le32tocpu(e->d_ino);
		size_t len = e->d_namelen;
		/* ensure that the name is short enough */
		if(len >= NAME_MAX)
			return false;

		/* now read the name */
		if(fread(e->d_name,1,len,dir) > 0) {
			/* if the record is longer, we have to skip the stuff until the next record */
			if(e->d_reclen - DIRE_SIZE > len) {
				len = (e->d_reclen - DIRE_SIZE - len);
				if(fseek(dir,len,SEEK_CUR) < 0)
					return false;
			}

			/* ensure that it is null-terminated */
			e->d_name[e->d_namelen] = '\0';
			return true;
		}
	}

	return false;
}
