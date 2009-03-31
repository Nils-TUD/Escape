/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <esc/common.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/env.h>
#include <esc/heap.h>
#include <string.h>

/* the dir-entry in which we'll write */
typedef struct {
	sDirEntry header;
	char name[MAX_NAME_LEN + 1];
} __attribute__((packed)) sDirEntryRead;

char *abspath(const char *path) {
	static char apath[MAX_PATH_LEN + 1];
	char *current,*curtemp,*pathtemp,*p;
	u32 layer,pos;

	/* skip namespace */
	p = (char*)path;
	pos = strchri(p,':');
	if(*(p + pos) != '\0') {
		strncpy(apath,p,pos + 1);
		apath[pos + 1] = '\0';
		pathtemp = apath + pos + 1;
		p += pos + 1;
	}
	else {
		strncpy(apath,"file:",5);
		apath[5] = '\0';
		pathtemp = apath + 5;
	}

	layer = 0;
	if(*p != '/') {
		current = getEnv("CWD");
		/* copy current to path */
		/* skip file:/ */
		*pathtemp++ = '/';
		curtemp = current + 6;
		while(*curtemp) {
			if(*curtemp == '/')
				layer++;
			*pathtemp = *curtemp;
			pathtemp++;
			curtemp++;
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
				do {
					pathtemp--;
				}
				while(*pathtemp != '/');
				*pathtemp = '\0';
				layer--;
			}
			p += 3;
		}
		else {
			/* append to path */
			*pathtemp++ = '/';
			strncpy(pathtemp,p,pos);
			pathtemp[pos] = '\0';
			pathtemp += pos;
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
	*pathtemp++ = '/';
	*pathtemp = '\0';
	return apath;
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

sDirEntry *readdir(tFD dir) {
	static sDirEntryRead dirEntry;
	u32 len;

	/* TODO a lot of improvement is needed here ;) */

	if(read(dir,(u8*)&dirEntry,sizeof(sDirEntry)) > 0) {
		len = dirEntry.header.nameLen;
		if(len >= MAX_NAME_LEN)
			return NULL;

		if(read(dir,(u8*)dirEntry.name,len) > 0) {
			/* if the record is longer, we have to read the remaining stuff to somewhere :( */
			/* TODO maybe we should introduce a read-to-null? (if buffer = NULL, throw data away?) */
			if(dirEntry.header.recLen - sizeof(sDirEntry) > len) {
				len = (dirEntry.header.recLen - sizeof(sDirEntry) - len);
				u8 *tmp = (u8*)malloc(len);
				if(tmp == NULL)
					return NULL;
				read(dir,tmp,len);
				free(tmp);
			}

			/* ensure that it is null-terminated */
			dirEntry.name[dirEntry.header.nameLen] = '\0';
			return (sDirEntry*)&dirEntry;
		}
	}

	return NULL;
}

void closedir(tFD dir) {
	close(dir);
}
