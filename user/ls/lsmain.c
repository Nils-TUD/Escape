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
#include <esc/fileio.h>
#include <esc/env.h>
#include <esc/date.h>
#include <stdlib.h>

#define ARRAY_INC_SIZE		8
#define CONSOLE_WIDTH		80

/* flags */
#define LS_FL_ALL			1
#define LS_FL_LONG			2
#define LS_FL_INODE			4

/* for calculating the widths of the fields */
#define WIDTHS_COUNT		6
#define W_INODE				0
#define W_LINKCOUNT			1
#define W_UID				2
#define W_GID				3
#define W_SIZE				4
#define W_NAME				5

/* internal data-structure for storing all attributes we want to display about a file/folder */
typedef struct {
	tInodeNo inodeNo;
	u16 uid;
	u16 gid;
	u16 mode;
	u16 linkCount;
	u32 modifytime;
	s32 size;
	char name[MAX_NAME_LEN + 1];
} sFullDirEntry;

static void printMode(u16 mode);
static s32 compareEntries(const void *a,const void *b);
static void printPerm(u16 mode,u16 flags,char c);
static sFullDirEntry **getEntries(const char *path,u16 flags,u32 *count);
static void freeEntries(sFullDirEntry **entries,u32 count);

int main(int argc,char *argv[]) {
	char *path = NULL;
	char *str;
	char dateStr[20];
	u32 widths[WIDTHS_COUNT] = {0};
	u32 i,pos,x,count,flags = 0;
	sFullDirEntry **entries,*entry;
	sDate date;

	/* parse args */
	for(i = 1; (s32)i < argc; i++) {
		if(*argv[i] == '-') {
			str = argv[i] + 1;
			while(*str) {
				switch(*str) {
					case 'a':
						flags |= LS_FL_ALL;
						break;
					case 'l':
						flags |= LS_FL_LONG;
						break;
					case 'i':
						flags |= LS_FL_INODE;
						break;
				}
				str++;
			}
		}
		else
			path = abspath(argv[argc - 1]);
	}

	/* path not provided? so use CWD */
	if(path == NULL) {
		path = getEnv("CWD");
		if(path == NULL) {
			printe("Unable to get CWD");
			return EXIT_FAILURE;
		}
	}

	/* get entries */
	entries = getEntries(path,flags,&count);
	if(entries == NULL)
		return EXIT_FAILURE;

	/* sort */
	qsort(entries,count,sizeof(sFullDirEntry*),compareEntries);

	/* calc widths */
	pos = 0;
	for(i = 0; i < count; i++) {
		if(!(flags & LS_FL_LONG)) {
			if((x = strlen(entries[i]->name)) > widths[W_NAME])
				widths[W_NAME] = x;
		}
		if(flags & LS_FL_INODE) {
			if((x = getnwidth(entries[i]->inodeNo)) > widths[W_INODE])
				widths[W_INODE] = x;
		}
		if(flags & LS_FL_LONG) {
			if((x = getuwidth(entries[i]->linkCount,10)) > widths[W_LINKCOUNT])
				widths[W_LINKCOUNT] = x;
			if((x = getuwidth(entries[i]->uid,10)) > widths[W_UID])
				widths[W_UID] = x;
			if((x = getuwidth(entries[i]->gid,10)) > widths[W_GID])
				widths[W_GID] = x;
			if((x = getnwidth(entries[i]->size)) > widths[W_SIZE])
				widths[W_SIZE] = x;
		}
	}

	/* display */
	for(i = 0; i < count; i++) {
		entry = entries[i];

		if(flags & LS_FL_LONG) {
			if(flags & LS_FL_INODE)
				printf("%*d ",widths[W_INODE],entry->inodeNo);
			printMode(entry->mode);
			printf("%*u ",widths[W_LINKCOUNT],entry->linkCount);
			printf("%*u ",widths[W_UID],entry->uid);
			printf("%*u ",widths[W_GID],entry->gid);
			printf("%*d ",widths[W_SIZE],entry->size);
			getDateOf(&date,entry->modifytime);
			dateToString(dateStr,20,"%Y-%m-%d %H:%M",&date);
			printf("%s ",dateStr);
			if(MODE_IS_DIR(entry->mode))
				printf("\033f\x09%s\033r\x0",entry->name);
			else if(entry->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
				printf("\033f\x02%s\033r\x0",entry->name);
			else
				printf("%s",entry->name);
			printf("\n");
		}
		else {
			/* if the entry does not fit on the line, use next */
			if(pos + widths[W_NAME] + widths[W_INODE] + 2 >= CONSOLE_WIDTH) {
				printf("\n");
				pos = 0;
			}
			if(flags & LS_FL_INODE)
				printf("%*d ",widths[W_INODE],entry->inodeNo);
			if(MODE_IS_DIR(entry->mode))
				printf("\033f\x09%-*s\033r\x0",widths[W_NAME] + 1,entry->name);
			else if(entry->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
				printf("\033f\x02%-*s\033r\x0",widths[W_NAME] + 1,entry->name);
			else
				printf("%-*s",widths[W_NAME] + 1,entry->name);
			pos += widths[W_NAME] + widths[W_INODE] + 2;
		}
	}

	if(!(flags & LS_FL_LONG))
		printf("\n");

	freeEntries(entries,count);
	return EXIT_SUCCESS;
}

static s32 compareEntries(const void *a,const void *b) {
	sFullDirEntry *ea = *(sFullDirEntry**)a;
	sFullDirEntry *eb = *(sFullDirEntry**)b;
	if(MODE_IS_DIR(ea->mode) == MODE_IS_DIR(eb->mode))
		return strcmp(ea->name,eb->name);
	if(MODE_IS_DIR(ea->mode))
		return -1;
	return 1;
}

static sFullDirEntry **getEntries(const char *path,u16 flags,u32 *count) {
	tFD dd;
	sDirEntry *de;
	sFullDirEntry *fde;
	sFileInfo info;
	char *fpath;
	u32 pathLen;
	u32 pos = 0;
	u32 size = ARRAY_INC_SIZE;
	sFullDirEntry **entries = (sFullDirEntry**)malloc(size * sizeof(sFullDirEntry*));
	if(entries == NULL) {
		printe("Unable to allocate memory for dir-entries");
		return NULL;
	}

	/* allocate mem for path */
	pathLen = strlen(path);
	fpath = (char*)malloc(pathLen + MAX_NAME_LEN + 1);
	if(fpath == NULL) {
		printe("Unable to allocate memory for file-path");
		return NULL;
	}
	strcpy(fpath,path);

	if((dd = opendir(path)) >= 0) {
		while((de = readdir(dd)) != NULL) {
			/* skip "." and ".." if -a is not enabled */
			if(!(flags & LS_FL_ALL) && (strcmp(de->name,".") == 0 || strcmp(de->name,"..") == 0))
				continue;

			/* retrieve information */
			strcpy(fpath + pathLen,de->name);
			if(getFileInfo(fpath,&info) < 0) {
				printe("Unable to get file-info for '%s'",fpath);
				freeEntries(entries,pos);
				free(fpath);
				closedir(dd);
				return NULL;
			}

			/* increase array-size? */
			if(pos >= size) {
				size += ARRAY_INC_SIZE;
				entries = (sFullDirEntry**)realloc(entries,size * sizeof(sFullDirEntry*));
				if(entries == NULL) {
					printe("Unable to reallocate memory for dir-entries");
					free(fpath);
					closedir(dd);
					return NULL;
				}
			}

			/* create entry */
			fde = (sFullDirEntry*)malloc(sizeof(sFullDirEntry));
			if(fde == NULL) {
				printe("Unable to allocate memory for dir-entry");
				freeEntries(entries,pos);
				free(fpath);
				closedir(dd);
				return NULL;
			}

			/* set content */
			fde->gid = info.gid;
			fde->uid = info.uid;
			fde->size = info.size;
			fde->inodeNo = info.inodeNo;
			fde->linkCount = info.linkCount;
			fde->mode = info.mode;
			fde->modifytime = info.modifytime;
			strcpy(fde->name,de->name);

			entries[pos++] = fde;
		}
		closedir(dd);
	}
	else {
		printe("Unable to open '%s'",path);
		freeEntries(entries,pos);
		free(fpath);
		return NULL;
	}

	*count = pos;
	return entries;
}

static void freeEntries(sFullDirEntry **entries,u32 count) {
	u32 i;
	for(i = 0; i < count; i++)
		free(entries[i]);
	free(entries);
}

static void printMode(u16 mode) {
	printPerm(MODE_IS_DIR(mode),1,'d');
	printPerm(mode,MODE_OWNER_READ,'r');
	printPerm(mode,MODE_OWNER_WRITE,'w');
	printPerm(mode,MODE_OWNER_EXEC,'x');
	printPerm(mode,MODE_GROUP_READ,'r');
	printPerm(mode,MODE_GROUP_WRITE,'w');
	printPerm(mode,MODE_GROUP_EXEC,'x');
	printPerm(mode,MODE_OTHER_READ,'r');
	printPerm(mode,MODE_OTHER_WRITE,'w');
	printPerm(mode,MODE_OTHER_EXEC,'x');
	printc(' ');
}

static void printPerm(u16 mode,u16 flags,char c) {
	if((mode & flags) != 0)
		printc(c);
	else
		printc('-');
}
