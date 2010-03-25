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
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/heap.h>
#include <esc/lock.h>
#include <string.h>

/* for the directory-cache */
#define CACHE_SIZE	512
#define DIRE_SIZE	(sizeof(sDirEntry) - (MAX_NAME_LEN + 1))

/**
 * Reads the next CACHE_SIZE bytes from fd and puts it into the cache
 *
 * @param fd the dir-descriptor
 */
static void incCache(tFD fd);

/* We provide a little cache for reading directories so that we don't need so many read()-calls.
 * This gives us much better performance because partly the directory-content is generated on
 * demand and if we're reading from the real filesystem we have to make a request to fs. */
/* But the cache is just usable for one directory. If the user-process opens another directory
 * in parallel we can't use the cache. */
static tULock dirLock = 0;
static tFD cfd = -1;
static char *cache = NULL;
static u32 cpos = 0;
static u32 csize = 0;

tFD opendir(const char *path) {
	tFD fd = open(path,IO_READ);
	if(fd >= 0) {
		locku(&dirLock);
		/* if the cache is in use, leave it alone */
		if(cache != NULL) {
			unlocku(&dirLock);
			return fd;
		}
		/* create cache */
		cache = (char*)malloc(CACHE_SIZE);
		/* if it fails, read without cache */
		if(cache == NULL) {
			unlocku(&dirLock);
			return fd;
		}
		/* read the first bytes */
		cpos = 0;
		csize = read(fd,cache,CACHE_SIZE);
		if((s32)csize < 0) {
			unlocku(&dirLock);
			close(fd);
			return (s32)csize;
		}
		cfd = fd;
		unlocku(&dirLock);
	}
	return fd;
}

bool readdir(sDirEntry *e,tFD dir) {
	u32 len;

	/* if the cache is ours, use it */
	locku(&dirLock);
	if(dir == cfd && cache) {
		/* check if we have to read more */
		sDirEntry *ec = (sDirEntry*)(cache + cpos);
		if(cpos >= csize || csize - cpos < DIRE_SIZE || csize - cpos < DIRE_SIZE + ec->nameLen)
			incCache(dir);
		if(cache) {
			/* rebuild pointer because of realloc */
			ec = (sDirEntry*)(cache + cpos);
			/* check if we've read enough */
			if(csize - cpos >= DIRE_SIZE + ec->nameLen) {
				/* copy to e and move to next */
				len = ec->nameLen;
				if(len == 0 || len >= MAX_NAME_LEN) {
					unlocku(&dirLock);
					return false;
				}
				memcpy(e,ec,DIRE_SIZE + len);
				e->name[len] = '\0';
				if(e->recLen - DIRE_SIZE > len)
					len = e->recLen;
				else
					len += DIRE_SIZE;
				cpos += len;
				unlocku(&dirLock);
				return true;
			}
		}
		unlocku(&dirLock);
		return false;
	}
	unlocku(&dirLock);

	/* default way; read the entry without name first */
	if(read(dir,(u8*)e,DIRE_SIZE) > 0) {
		len = e->nameLen;
		/* ensure that the name is short enough */
		if(len >= MAX_NAME_LEN)
			return false;

		/* now read the name */
		if(read(dir,(u8*)e->name,len) > 0) {
			/* if the record is longer, we have to skip the stuff until the next record */
			if(e->recLen - DIRE_SIZE > len) {
				len = (e->recLen - DIRE_SIZE - len);
				if(seek(dir,len,SEEK_CUR) < 0)
					return false;
			}

			/* ensure that it is null-terminated */
			e->name[e->nameLen] = '\0';
			return true;
		}
	}

	return false;
}

void closedir(tFD dir) {
	/* free cache if it is our */
	locku(&dirLock);
	if(cfd == dir) {
		cfd = -1;
		free(cache);
		cache = NULL;
	}
	unlocku(&dirLock);
	close(dir);
}

static void incCache(tFD fd) {
	s32 res;
	char *dup;
	u32 nsize = MAX(cpos + CACHE_SIZE,csize + CACHE_SIZE);
	dup = (char*)realloc(cache,nsize);
	if(!dup)
		free(cache);
	cache = dup;
	if(cache) {
		res = read(fd,cache + csize,nsize - csize);
		if(res >= 0) {
			csize += res;
			return;
		}
		free(cache);
		cache = NULL;
	}
}
