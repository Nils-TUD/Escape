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

#include <sys/common.h>
#include <stdio.h>
#include <stdlib.h>

#include "iobuf.h"

FILE *fattach(int fd,const char *mode) {
	uint flags = parsemode(mode);
	if((flags & O_ACCMODE) == 0 || (flags & (O_APPEND | O_CREAT | O_EXCL | O_LONELY | O_TRUNC)))
		return NULL;
	flags |= O_NOCLOSE;

	/* create file */
	FILE *f;
	if(!(f = bcreate(fd,flags,NULL,IN_BUFFER_SIZE,OUT_BUFFER_SIZE,false))) {
		free(f);
		return NULL;
	}
	benqueue(f);
	return f;
}
