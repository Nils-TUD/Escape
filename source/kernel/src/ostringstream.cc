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

#include <sys/ostringstream.h>

void OStringStream::writec(char c) {
	if(size != (size_t)-1) {
		if(dynamic) {
			if(len >= size) {
				size *= 2;
				char *dup = (char*)Cache::realloc(str,size * sizeof(char));
				if(!dup) {
					/* make end visible */
					str[len - 1] = 0xBA;
					size = (size_t)-1;
					return;
				}
				else
					str = dup;
			}
		}
		if(str && (dynamic || c == '\0' || len + 1 < size)) {
			str[len] = c;
			if(c != '\0')
				len++;
		}
	}
}
