/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
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

#define LS_FL_ALL			1
#define LS_FL_LONG			2

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
	u32 i,pos,x,maxWidth,count,flags = 0;
	sFullDirEntry **entries,*entry;
	sDate date;

	/* parse args */
	for(i = 1; i < argc; i++) {
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
			printe("Unable to get CWD\n");
			return EXIT_FAILURE;
		}
	}

	/* get entries */
	entries = getEntries(path,flags,&count);
	if(entries == NULL)
		return EXIT_FAILURE;

	/* sort */
	qsort(entries,count,sizeof(sFullDirEntry*),compareEntries);

	/* init short-mode */
	if(!(flags & LS_FL_LONG)) {
		pos = 0;
		maxWidth = 0;
		for(i = 0; i < count; i++) {
			if((x = strlen(entries[i]->name)) > maxWidth)
				maxWidth = x;
		}
		/* one space between the dir-entries is good :) */
		maxWidth++;
	}

	/* display */
	for(i = 0; i < count; i++) {
		entry = entries[i];

		if(flags & LS_FL_LONG) {
			printMode(entry->mode);
			printf("%3d ",entry->linkCount);
			printf("%2d ",entry->uid);
			printf("%2d ",entry->gid);
			printf("%10d ",entry->size);
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
			if(pos + maxWidth >= CONSOLE_WIDTH) {
				printf("\n");
				pos = 0;
			}
			if(MODE_IS_DIR(entry->mode))
				printf("\033f\x09%-*s\033r\x0",maxWidth,entry->name);
			else if(entry->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
				printf("\033f\x02%-*s\033r\x0",maxWidth,entry->name);
			else
				printf("%-*s",maxWidth,entry->name);
			pos += maxWidth;
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
		printe("Unable to open '%s'\n",path);
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
