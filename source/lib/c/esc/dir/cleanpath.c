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
	char *curtemp,*pathtemp,*p;
	int layer,pos;
	size_t count = 0;

	p = (char*)src;
	pathtemp = dst;
	layer = 0;
	if(*p != '/') {
		char envPath[MAX_PATH_LEN + 1];
		if(getenvto(envPath,MAX_PATH_LEN + 1,"CWD") < 0)
			return count;
		if(dstSize < strlen(envPath))
			return count;
		/* copy current to path */
		*pathtemp++ = '/';
		*pathtemp = '\0';
		curtemp = envPath + 1;
		while(*curtemp) {
			if(*curtemp == '/')
				layer++;
			*pathtemp++ = *curtemp++;
		}
		/* append '/' */
		if(pathtemp[-1] != '/') {
			*pathtemp++ = '/';
			layer++;
		}
		count = pathtemp - dst;
	}
	else {
		*pathtemp++ = '/';
		count++;
		/* skip leading '/' */
		do {
			p++;
		}
		while(*p == '/');
	}

	while(*p) {
		pos = strchri(p,'/');

		/* simply skip '.' */
		if(pos == 1 && p[0] == '.')
			p += 2;
		/* one layer back */
		else if(pos == 2 && p[0] == '.' && p[1] == '.') {
			if(layer > 0) {
				char *start = pathtemp;
				/* to last slash */
				pathtemp -= 2;
				while(*pathtemp != '/')
					pathtemp--;
				*++pathtemp = '\0';
				count -= start - pathtemp;
				layer--;
			}
			p += 3;
		}
		else {
			if(dstSize - count < (size_t)(pos + 2))
				return count;
			/* append to path */
			strncpy(pathtemp,p,pos);
			pathtemp[pos] = '/';
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
	*pathtemp = '\0';
	return count;
}
