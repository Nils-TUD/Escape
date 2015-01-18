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
#include <ctype.h>
#include <stddef.h>
#include <string.h>

int strncasecmp(const char *str1,const char *str2,size_t count) {
	char c1 = 0,c2 = 0;
	ssize_t rem = count;
	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(rem > 0 && (c1 = tolower(*str1)) && (c2 = tolower(*str2))) {
		if(c1 != c2) {
			if(c1 < c2)
				return -1;
			return 1;
		}
		str1++;
		str2++;
		rem--;
	}
	if(rem <= 0)
		return 0;
	if(c1 && !c2)
		return 1;
	return -1;
}
