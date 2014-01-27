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

#include <stddef.h>
#include <assert.h>
#include <string.h>

size_t strspn(const char *str1,const char *str2) {
	size_t count = 0;

	vassert(str1 != NULL,"str1 == NULL");
	vassert(str2 != NULL,"str2 == NULL");

	while(*str1) {
		if(strchr(str2,*str1++) == NULL)
			return count;
		count++;
	}
	return count;
}
