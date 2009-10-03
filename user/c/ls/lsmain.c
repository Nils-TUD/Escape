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
#include <esc/cmdargs.h>
#include <stdlib.h>
#include <width.h>

#define DATE_LEN			(SSTRLEN("2009-09-09 14:12") + 1)
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
static sFullDirEntry *getEntry(sFileInfo *info,const char *name);
static void freeEntries(sFullDirEntry **entries,u32 count);

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-lia] [<path>]\n",name);
	fprintf(stderr,"	-l: long listing\n");
	fprintf(stderr,"	-i: print inode-numbers\n");
	fprintf(stderr,"	-a: print also '.' and '..'\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	bool pathGiven = false;
	char *path;
	char *str;
	char dateStr[DATE_LEN];
	u32 widths[WIDTHS_COUNT] = {0};
	u32 i,pos,x,count,flags = 0;
	sFullDirEntry **entries,*entry;
	sDate date;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(path == NULL)
		error("Not enough mem for path");

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
					default:
						usage(argv[0]);
						break;
				}
				str++;
			}
		}
		else {
			if(pathGiven)
				usage(argv[0]);
			abspath(path,MAX_PATH_LEN + 1,argv[argc - 1]);
			pathGiven = true;
		}
	}

	/* path not provided? so use CWD */
	if(!pathGiven && !getEnv(path,MAX_PATH_LEN + 1,"CWD"))
		error("Unable to get CWD");

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
			dateToString(dateStr,DATE_LEN,"%Y-%m-%d %H:%M",&date);
			printf("%s ",dateStr);
			if(MODE_IS_DIR(entry->mode))
				printf("\033[co;9]%s\033[co]",entry->name);
			else if(entry->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
				printf("\033[co;2]%s\033[co]",entry->name);
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
				printf("\033[co;9]%-*s\033[co]",widths[W_NAME] + 1,entry->name);
			else if(entry->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
				printf("\033[co;2]%-*s\033[co]",widths[W_NAME] + 1,entry->name);
			else
				printf("%-*s",widths[W_NAME] + 1,entry->name);
			pos += widths[W_NAME] + widths[W_INODE] + 2;
		}
	}

	if(!(flags & LS_FL_LONG))
		printf("\n");

	freeEntries(entries,count);
	free(path);
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
	sDirEntry de;
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

	if(stat(path,&info) < 0) {
		printe("Unable to get file-info for '%s'",path);
		free(entries);
		return NULL;
	}

	/* allocate mem for path */
	pathLen = strlen(path);
	fpath = (char*)malloc(pathLen + MAX_NAME_LEN + 1);
	if(fpath == NULL) {
		printe("Unable to allocate memory for file-path");
		free(entries);
		return NULL;
	}
	strcpy(fpath,path);

	if(MODE_IS_DIR(info.mode)) {
		if((dd = opendir(path)) >= 0) {
			while(readdir(&de,dd)) {
				/* skip "." and ".." if -a is not enabled */
				if(!(flags & LS_FL_ALL) && (strcmp(de.name,".") == 0 || strcmp(de.name,"..") == 0))
					continue;

				/* retrieve information */
				strcpy(fpath + pathLen,de.name);
				if(stat(fpath,&info) < 0) {
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
				fde = getEntry(&info,de.name);
				if(fde == NULL) {
					freeEntries(entries,pos);
					free(fpath);
					closedir(dd);
					return NULL;
				}
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
	}
	else {
		/* determine name */
		char *lastSlash = strrchr(path,'/');
		if(lastSlash != NULL) {
			if(*(lastSlash + 1) == '\0') {
				*lastSlash = '\0';
				lastSlash = strrchr(path,'/');
				if(lastSlash == NULL)
					lastSlash = (char*)path;
				else
					lastSlash++;
			}
			else
				lastSlash++;
		}
		else
			lastSlash = (char*)path;

		fde = getEntry(&info,lastSlash);
		if(fde == NULL) {
			freeEntries(entries,pos);
			free(fpath);
			return NULL;
		}
		entries[pos++] = fde;
	}

	*count = pos;
	return entries;
}

static sFullDirEntry *getEntry(sFileInfo *info,const char *name) {
	sFullDirEntry *fde = (sFullDirEntry*)malloc(sizeof(sFullDirEntry));
	if(fde == NULL) {
		printe("Unable to allocate memory for dir-entry");
		return NULL;
	}

	/* set content */
	fde->gid = info->gid;
	fde->uid = info->uid;
	fde->size = info->size;
	fde->inodeNo = info->inodeNo;
	fde->linkCount = info->linkCount;
	fde->mode = info->mode;
	fde->modifytime = info->modifytime;
	strcpy(fde->name,name);
	return fde;
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
