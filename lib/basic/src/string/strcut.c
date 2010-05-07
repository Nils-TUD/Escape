/**
 * $Id: strcut.c 332 2009-09-17 09:39:40Z nasmussen $
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

char *strcut(char *str,u32 count) {
	char *res = str;

	vassert(str != NULL,"str == NULL");

	if(count > 0) {
		str += count;
		while(*str) {
			*(str - count) = *str;
			str++;
		}
		*(str - count) = '\0';
	}
	return res;
}
