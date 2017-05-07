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

#include <assert.h>
#include <stddef.h>
#include <string.h>

bool strmatch(const char *pattern,const char *str) {
	size_t plen = strlen(pattern);
	size_t slen = strlen(str);

	const char *firstStar = strchr(pattern,'*');
	if(firstStar == NULL)
		return strcmp(pattern,str) == 0;

	/* does the beginning match? */
	const char *lastStar = strrchr(pattern,'*');
	if(firstStar != pattern) {
		if(strncmp(str,pattern,firstStar - pattern) != 0)
			return false;
	}
	
	/* does the end match? */
	if(lastStar[1] != '\0') {
		size_t cmplen = pattern + plen - lastStar - 1;
		if(strncmp(lastStar + 1,str + slen - cmplen,cmplen) != 0)
			return false;
	}

	/* now check whether the parts between the stars match */
	str += firstStar - pattern;
	slen -= firstStar - pattern;
	while(1) {
		char *match;
		const char *start = firstStar + 1;
		firstStar = strchr(start,'*');
		if(firstStar == NULL)
			break;

		match = memmem(str,slen,start,firstStar - start);
		if(match == NULL)
			return false;
		str = match + (firstStar - start);
		slen -= (match - str) + (firstStar - start);
	}
	return true;
}
