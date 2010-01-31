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
#include <string.h>

#ifdef IN_KERNEL
#	include <mem/kheap.h>
#	define malloc kheap_alloc
#else
#	include <esc/heap.h>
#endif

char *strndup(const char *s,u32 n) {
	u32 len = strlen(s);
	len = MIN(len,n);
	char *res = (char*)malloc(len + 1);
	if(res) {
		strncpy(res,s,len);
		res[len] = '\0';
	}
	return res;
}
