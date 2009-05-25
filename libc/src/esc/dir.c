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

u32 abspath(char *dst,u32 dstSize,const char *src) {
	char *curtemp,*pathtemp,*p;
	u32 layer,pos;
	u32 count = 0;

	/* skip namespace */
	p = (char*)src;
	pos = strchri(p,':');
	if(*(p + pos) != '\0') {
		if(dstSize < pos + 2)
			return count;
		strncpy(dst,p,pos + 1);
		dst[pos + 1] = '\0';
		pathtemp = dst + pos + 1;
		p += pos + 1;
		count += pos + 1;
	}
	else {
		if(dstSize < 6)
			return count;
		strncpy(dst,"file:",5);
		dst[5] = '\0';
		pathtemp = dst + 5;
		count += 5;
	}

	layer = 0;
	if(*p != '/') {
		char envPath[MAX_PATH_LEN + 1];
		if(!getEnv(&envPath,MAX_PATH_LEN + 1,"CWD"))
			return count;
		/* we'll append strlen(envPath) without "file:/" and trailing slash, but need a
		 * null-termination */
		if(dstSize - count < strlen(envPath) - 6)
			return count;
		/* copy current to path */
		/* skip file:/ */
		*pathtemp++ = '/';
		curtemp = envPath + 6;
		while(*curtemp) {
			if(*curtemp == '/')
				layer++;
			*pathtemp++ = *curtemp++;
		}
		/* remove '/' at the end */
		pathtemp--;
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
	if(*p == ':' || len == 0)
		return;

	/* remove last path component */
	while(*p != '/')
		p--;

	/* set new end */
	*(p + 1) = '\0';
}

tFD opendir(const char *path) {
	return open(path,IO_READ);
}

bool readdir(sDirEntry *e,tFD dir) {
	u32 len;

	/* TODO a lot of improvement is needed here ;) */

	if(read(dir,(u8*)e,sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) > 0) {
		len = e->nameLen;
		if(len >= MAX_NAME_LEN)
			return false;

		if(read(dir,(u8*)e->name,len) > 0) {
			/* if the record is longer, we have to read the remaining stuff to somewhere :( */
			/* TODO maybe we should introduce a read-to-null? (if buffer = NULL, throw data away?) */
			if(e->recLen - (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) > len) {
				len = (e->recLen - (sizeof(sDirEntry) - (MAX_NAME_LEN + 1)) - len);
				u8 *tmp = (u8*)malloc(len);
				if(tmp == NULL)
					return false;
				read(dir,tmp,len);
				free(tmp);
			}

			/* ensure that it is null-terminated */
			e->name[e->nameLen] = '\0';
			return true;
		}
	}

	return false;
}

void closedir(tFD dir) {
	close(dir);
}
