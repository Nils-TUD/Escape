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
#include <sys/stat.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iobuf.h"

#define NAME_LEN	8

FILE *tmpfile(void) {
	static bool initialized = false;
	if(!initialized) {
		srand(rdtsc());
		initialized = true;
	}

	int fd;
	const uint flags = O_RDWR | O_EXCL | O_LONELY;
	char path[MAX_PATH_LEN] = "/tmp/";
	do {
		for(int i = 0; i < NAME_LEN; ++i)
			path[i + 5] = 'a' + (rand() % ('z' - 'a'));
		path[NAME_LEN + 5] = '\0';

		fd = create(path,flags,0644);
	}
	while(fd == -EEXIST);
	if(fd < 0)
		return NULL;

	/* unlink it now to make it invisible and destroy it on close */
	sassert(unlink(path) == 0);

	FILE *f;
	if(!(f = bcreate(fd,flags,NULL,IN_BUFFER_SIZE,OUT_BUFFER_SIZE,false))) {
		close(fd);
		free(f);
		return NULL;
	}
	benqueue(f);
	return f;
}
