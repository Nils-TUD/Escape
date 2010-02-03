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

#include <stddef.h>
#include <assert.h>
#include <string.h>

bool strmatch(const char *pattern,const char *str) {
	char *lastStar,*firstStar = strchr(pattern,'*');
	if(firstStar == NULL)
		return strcmp(pattern,str) == 0;
	lastStar = strrchr(pattern,'*');
	/* does the beginning match? */
	if(firstStar != pattern) {
		if(strncmp(str,pattern,firstStar - pattern) != 0)
			return false;
	}
	/* does the end match? */
	if(lastStar[1] != '\0') {
		u32 plen = strlen(pattern);
		u32 slen = strlen(str);
		u32 cmplen = pattern + plen - lastStar - 1;
		if(strncmp(lastStar + 1,str + slen - cmplen,cmplen) != 0)
			return false;
	}

	/* now check wether the parts between the stars match */
	str += firstStar - pattern;
	while(1) {
		char *match;
		char *start = firstStar + 1;
		firstStar = strchr(start,'*');
		if(firstStar == NULL)
			break;

		*firstStar = '\0';
		match = strstr(str,start);
		*firstStar = '*';
		if(match == NULL)
			return false;
		str = match + (firstStar - start);
	}
	return true;
}
