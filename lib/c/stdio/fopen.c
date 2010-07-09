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

#include <esc/common.h>
#include <esc/exceptions/io.h>
#include <esc/exceptions/outofmemory.h>
#include <esc/io/iofilestream.h>
#include <stdio.h>

FILE *fopen(const char *filename,const char *mode) {
	FILE *res = NULL;
	TRY {
		/* parse mode */
		char c;
		u8 flags = 0;
		while((c = *mode++)) {
			switch(c) {
				case 'r':
					flags |= IO_READ;
					break;
				case 'w':
					flags |= IO_WRITE | IO_CREATE | IO_TRUNCATE;
					break;
				case '+':
					if(flags & IO_READ)
						flags |= IO_WRITE;
					else if(flags & IO_WRITE)
						flags |= IO_READ;
					break;
				case 'a':
					flags |= IO_APPEND | IO_WRITE;
					break;
			}
		}
		/* open stream */
		res = (FILE*)iofstream_open(filename,flags);
	}
	CATCH(IOException,e) {
		res = NULL;
	}
	CATCH(OutOfMemoryException,e) {
		res = NULL;
	}
	ENDCATCH
	return res;
}
