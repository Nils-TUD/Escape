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
#include <esc/dir.h>
#include <string.h>
#include <stdlib.h>

size_t cleanpath(char *dst,size_t dstSize,const char *src) {
	char tmp[MAX_PATH_LEN];
	char *p = abspath(tmp,sizeof(tmp),src);
	while(*p == '/')
		p++;

	int layer = 0;
	size_t count = 0;
	char *pathtemp = dst;
	while(*p) {
		int pos = strchri(p,'/');

		/* simply skip '.' */
		if(pos == 1 && p[0] == '.')
			p += 2;
		/* one layer back */
		else if(pos == 2 && p[0] == '.' && p[1] == '.') {
			if(layer > 0) {
				char *start = pathtemp;
				/* to last slash */
				while(*pathtemp != '/')
					pathtemp--;
				*pathtemp = '\0';
				count -= start - pathtemp;
				layer--;
			}
			p += 3;
		}
		else {
			if(dstSize - count < (size_t)(pos + 2))
				return count;
			/* append to path */
			pathtemp[0] = '/';
			strncpy(pathtemp + 1,p,pos);
			pathtemp[pos + 1] = '\0';
			pathtemp += pos + 1;
			count += pos + 1;
			p += pos + 1;
			layer++;
		}

		/* one step too far? */
		if(*(p - 1) == '\0')
			break;

		/* skip multiple '/' */
		while(*p == '/')
			p++;
	}

	/* terminate */
	if(dstSize - count < 2)
		return count;
	/* root? */
	if(pathtemp == dst) {
		*pathtemp++ = '/';
		count++;
	}
	*pathtemp = '\0';
	return count;
}
