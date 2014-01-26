/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/mem.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "files.h"

static FSFile *files = NULL;
static size_t fileSize = 0;

int files_open(size_t id,inode_t ino) {
	if(id >= fileSize) {
		size_t newSize = MAX((size_t)id + 1,fileSize + 32);
		FSFile *newFiles = realloc(files,newSize * sizeof(FSFile));
		if(!newFiles)
			return -ENOMEM;
		files = newFiles;
		fileSize = newSize;
	}
	files[id].ino = ino;
	files[id].shm = NULL;
	return 0;
}

FSFile *files_get(size_t id) {
	assert(id < fileSize);
	return files + id;
}

void files_close(size_t id) {
	assert(id < fileSize);
	if(files[id].shm)
		munmap(files[id].shm);
	files[id].ino = 0;
}
