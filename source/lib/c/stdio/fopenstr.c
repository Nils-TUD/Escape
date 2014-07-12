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
#include <sys/sllist.h>
#include "iobuf.h"
#include <stdio.h>
#include <stdlib.h>

FILE *fopenstr(char *buf,size_t size,const char *mode) {
	size_t rsize = 0,wsize = 0;
	uint flags = 0;
	if(*mode == 'r') {
		flags = O_RDONLY;
		rsize = size;
	}
	else if(*mode == 'w') {
		flags = O_WRONLY;
		wsize = size;
	}
	/* invalid mode? */
	if(flags == 0 || mode[1] != '\0')
		return NULL;

	/* create file */
	FILE *f;
	if(!(f = bcreate(-1,flags,buf,rsize,wsize,false)) || !sll_append(&iostreams,f)) {
		free(f);
		return NULL;
	}
	return f;
}
