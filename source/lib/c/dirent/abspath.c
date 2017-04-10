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
#include <assert.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

char *abspath(char *dst,size_t dstSize,const char *path) {
	assert(dstSize > 0);

	if(*path != '/') {
		/* translate "abc://def" to "/dev/abc/def" */
		const char *p = path;
		while(*p) {
			if(p[0] == ':' && p[1] == '/' && p[2] == '/') {
				size_t num = MIN(dstSize - 1,SSTRLEN("/dev/"));
				strncpy(dst,"/dev/",num);
				dstSize -= num;

				size_t slen = MIN(dstSize - 1,(size_t)(p - path));
				strncpy(dst + num,path,slen);
				dstSize -= slen;

				strnzcpy(dst + num + slen,p + 2,dstSize);
				return dst;
			}
			p++;
		}

		/* prepend CWD */
		size_t len = getenvto(dst,dstSize,"CWD");
		if(len < dstSize - 1 && dst[len - 1] != '/') {
			dst[len++] = '/';
			dst[len] = '\0';
		}
		strnzcpy(dst + len,path,dstSize - len);
		return dst;
	}
	return (char*)path;
}
