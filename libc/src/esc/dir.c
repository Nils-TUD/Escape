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
#include <esc/env.h>
#include <esc/heap.h>
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
static tFD cfd = -1;
static char *cache = NULL;
static u32 cpos = 0;
static u32 csize = 0;

u32 abspath(char *dst,u32 dstSize,const char *src) {
	char *curtemp,*pathtemp,*p;
	u32 layer,pos;
	u32 count = 0;

	p = (char*)src;
	pathtemp = dst;
	layer = 0;
	if(*p != '/') {
		char envPath[MAX_PATH_LEN + 1];
		if(!getEnv(envPath,MAX_PATH_LEN + 1,"CWD"))
			return count;
		if(dstSize < strlen(envPath))
			return count;
		/* copy current to path */
		*pathtemp++ = '/';
		curtemp = envPath + 1;
		while(*curtemp) {
			if(*curtemp == '/')
				layer++;
			*pathtemp++ = *curtemp++;
		}
		/* remove '/' at the end */
		pathtemp--;
		count = pathtemp - dst;
	}
	else {
		/* skip leading '/' */
		do {
			p++;
		}
		while(*p == '/');
	}

	while(*p) {
		pos = strchri(p,'/');

		/* simply skip '.' */
		if(pos == 1 && strncmp(p,".",1) == 0)
			p += 2;
		/* one layer back */
		else if(pos == 2 && strncmp(p,"..",2) == 0) {
			if(layer > 0) {
				char *start = pathtemp;
				do {
					pathtemp--;
				}
				while(*pathtemp != '/');
				*pathtemp = '\0';
				count -= start - pathtemp;
				layer--;
			}
			p += 3;
		}
		else {
			if(dstSize - count < pos + 2)
				return count;
			/* append to path */
			*pathtemp++ = '/';
			strncpy(pathtemp,p,pos);
			pathtemp[pos] = '\0';
			pathtemp += pos;
			count += pos + 1;
			p += pos + 1;
			layer++;
		}

		/* one step too far? */
		if(*(p - 1) == '\0')
			break;

		/* skip multiple '/' */
		while(*p == '/')
			p++;
	}

	/* terminate */
	if(dstSize - count < 2)
		return count;
	*pathtemp++ = '/';
	*pathtemp = '\0';
	count++;
	return count;
}

void dirname(char *path) {
	char *p;
	u32 len = strlen(path);

	p = path + len - 1;
	/* remove last '/' */
	if(*p == '/') {
		p--;
		len--;
	}

	/* nothing to remove? */
	if(len == 0)
		return;

	/* remove last path component */
	while(*p != '/')
		p--;

	/* set new end */
	*(p + 1) = '\0';
}

tFD opendir(const char *path) {
	tFD fd = open(path,IO_READ);
	if(fd >= 0) {
		/* if the cache is in use, leave it alone */
		if(cache != NULL)
			return fd;
		/* create cache */
		cache = (char*)malloc(CACHE_SIZE);
		/* if it fails, read without cache */
		if(cache == NULL)
			return fd;
		/* read the first bytes */
		cpos = 0;
		csize = read(fd,cache,CACHE_SIZE);
		if((s32)csize < 0) {
			close(fd);
			return (s32)csize;
		}
		cfd = fd;
	}
	return fd;
}

bool readdir(sDirEntry *e,tFD dir) {
	u32 len;

	/* if the cache is ours, use it */
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
				if(len >= MAX_NAME_LEN)
					return false;
				memcpy(e,ec,DIRE_SIZE + len);
				e->name[len] = '\0';
				if(e->recLen - DIRE_SIZE > len)
					len = e->recLen;
				else
					len += DIRE_SIZE;
				cpos += len;
				return true;
			}
		}
		return false;
	}

	/* default way; read the entry without name first */
	if(read(dir,(u8*)e,sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) > 0) {
		len = e->nameLen;
		/* ensure that the name is short enough */
		if(len >= MAX_NAME_LEN)
			return false;

		/* now read the name */
		if(read(dir,(u8*)e->name,len) > 0) {
			/* if the record is longer, we have to skip the stuff until the next record */
			if(e->recLen - (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) > len) {
				len = (e->recLen - (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) - len);
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
	if(cfd == dir) {
		cfd = -1;
		free(cache);
		cache = NULL;
	}
	close(dir);
}

static void incCache(tFD fd) {
	s32 res;
	u32 nsize = MAX(cpos + CACHE_SIZE,csize + CACHE_SIZE);
	cache = (char*)realloc(cache,nsize);
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
