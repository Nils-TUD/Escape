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
#include <esc/fileio.h>
#include <esc/io.h>
#include "fileiointern.h"

tFile *fopen(const char *filename,const char *mode) {
	char c;
	sIOBuffer *buf;
	u8 flags = 0;
	tFD fd;

	/* parse mode */
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
				/* TODO */
				break;
		}
	}

	/* open the file */
	fd = open(filename,flags);
	if(fd < 0)
		return NULL;

	/* create buffer for it */
	buf = bcreate(fd,flags);
	return (tFile*)buf;
}
