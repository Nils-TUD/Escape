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
#include <esc/proc.h>
#include <esc/dir.h>
#include <esc/date.h>
#include <esc/mem/heap.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/exceptions/io.h>
#include <esc/io/file.h>
#include <esc/io/console.h>
#include <esc/util/cmdargs.h>
#include <esc/util/vector.h>
#include <width.h>
#include <messages.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DATE_LEN			(SSTRLEN("2009-09-09 14:12") + 1)

#define F_LONG_SHIFT		0
#define F_INODE_SHIFT		1
#define F_ALL_SHIFT			2
#define F_DIRSIZE_SHIFT		3
#define F_DIRNUM_SHIFT		4

#define F_LONG				(1 << F_LONG_SHIFT)
#define F_INODE				(1 << F_INODE_SHIFT)
#define F_ALL				(1 << F_ALL_SHIFT)
#define F_DIRSIZE			(1 << F_DIRSIZE_SHIFT)
#define F_DIRNUM			(1 << F_DIRNUM_SHIFT)

/* for calculating the widths of the fields */
#define WIDTHS_COUNT		6
#define W_INODE				0
#define W_LINKCOUNT			1
#define W_UID				2
#define W_GID				3
#define W_SIZE				4
#define W_NAME				5

typedef struct {
	tInodeNo inodeNo;
	u16 mode;
	u16 linkCount;
	u16 uid;
	u16 gid;
	s32 size;
	u32 modifytime;
	char name[MAX_NAME_LEN];
} sLSFile;

static s32 compareEntries(const void *a,const void *b);
static sVector *getEntries(const char *path);
static sLSFile *buildFile(sFile *f);
static s32 getDirSize(sFile *d);
static void printMode(u16 mode);
static void printPerm(u16 mode,u16 flags,char c);

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-liasn] [<path>]\n",name);
	cerr->writef(cerr,"	-l: long listing\n");
	cerr->writef(cerr,"	-i: print inode-numbers\n");
	cerr->writef(cerr,"	-a: print also '.' and '..'\n");
	cerr->writef(cerr,"	-s: print size of directory-content instead of directory-entries (implies -l)\n");
	cerr->writef(cerr,"	-n: print number of directory-entries instead of their size (implies -l)\n");
	exit(EXIT_FAILURE);
}

static u32 flags;

int main(int argc,const char *argv[]) {
	char path[MAX_PATH_LEN];
	u32 widths[WIDTHS_COUNT] = {0};
	u32 pos,x;
	sLSFile *f;
	sVTSize consSize;
	sVector *entries = NULL;
	sCmdArgs *args = NULL;
	const char *filename;

	/* parse params */
	TRY {
		bool flong = 0,finode = 0,fall = 0,fdirsize,fdirnum;
		args = cmdargs_create(argc,argv,CA_MAX1_FREE);
		args->parse(args,"l i a s n",&flong,&finode,&fall,&fdirsize,&fdirnum);
		if(args->isHelp)
			usage(argv[0]);
		flags = (flong << F_LONG_SHIFT) | (finode << F_INODE_SHIFT) | (fall << F_ALL_SHIFT) |
			(fdirsize << F_DIRSIZE_SHIFT) | (fdirnum << F_DIRNUM_SHIFT);
		/* dirsize and dirnum imply long */
		if(flags & (F_DIRSIZE | F_DIRNUM))
			flags |= F_LONG;
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	filename = args->getFirstFree(args);
	if(filename)
		abspath(path,sizeof(path),filename);
	/* path not provided? so use CWD */
	else if(!getenvto(path,MAX_PATH_LEN + 1,"CWD"))
		error("Unable to get CWD");

	if(recvMsgData(STDIN_FILENO,MSG_VT_GETSIZE,&consSize,sizeof(sVTSize)) < 0)
		error("Unable to determine screensize");

	/* get entries and sort them */
	TRY {
		entries = getEntries(path);
		vec_sortCustom(entries,compareEntries);
	}
	CATCH(IOException,e) {
		error("Unable to read dir-entries: %s",e->toString(e));
	}
	ENDCATCH

	/* calc widths */
	pos = 0;
	{
		vforeach(entries,f) {
			if(flags & F_INODE) {
				if((x = getnwidth(f->inodeNo)) > widths[W_INODE])
					widths[W_INODE] = x;
			}
			if(flags & F_LONG) {
				if((x = getuwidth(f->linkCount,10)) > widths[W_LINKCOUNT])
					widths[W_LINKCOUNT] = x;
				if((x = getuwidth(f->uid,10)) > widths[W_UID])
					widths[W_UID] = x;
				if((x = getuwidth(f->gid,10)) > widths[W_GID])
					widths[W_GID] = x;
				if((x = getnwidth(f->size)) > widths[W_SIZE])
					widths[W_SIZE] = x;
			}
			else {
				if((x = strlen(f->name)) > widths[W_NAME])
					widths[W_NAME] = x;
			}
		}
	}

	/* display */
	{
		vforeach(entries,f) {
			if(flags & F_LONG) {
				if(flags & F_INODE)
					cout->writef(cout,"%*d ",widths[W_INODE],f->inodeNo);
				printMode(f->mode);
				cout->writef(cout,"%*u ",widths[W_LINKCOUNT],f->linkCount);
				cout->writef(cout,"%*u ",widths[W_UID],f->uid);
				cout->writef(cout,"%*u ",widths[W_GID],f->gid);
				cout->writef(cout,"%*d ",widths[W_SIZE],f->size);
				{
					char dateStr[DATE_LEN];
					sDate date = date_getOfTS(f->modifytime);
					date.format(&date,dateStr,DATE_LEN,"%Y-%m-%d %H:%M");
					cout->writef(cout,"%s ",dateStr);
				}
				if(MODE_IS_DIR(f->mode))
					cout->writef(cout,"\033[co;9]%s\033[co]",f->name);
				else if(f->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
					cout->writef(cout,"\033[co;2]%s\033[co]",f->name);
				else
					cout->writes(cout,f->name);
				cout->writec(cout,'\n');
			}
			else {
				/* if the entry does not fit on the line, use next */
				if(pos + widths[W_NAME] + widths[W_INODE] + 2 >= consSize.width) {
					cout->writec(cout,'\n');
					pos = 0;
				}
				if(flags & F_INODE)
					cout->writef(cout,"%*d ",widths[W_INODE],f->inodeNo);
				if(MODE_IS_DIR(f->mode))
					cout->writef(cout,"\033[co;9]%-*s\033[co]",widths[W_NAME] + 1,f->name);
				else if(f->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
					cout->writef(cout,"\033[co;2]%-*s\033[co]",widths[W_NAME] + 1,f->name);
				else
					cout->writef(cout,"%-*s",widths[W_NAME] + 1,f->name);
				pos += widths[W_NAME] + widths[W_INODE] + 2;
			}
		}
	}

	if(!(flags & F_LONG))
		cout->writec(cout,'\n');

	vec_destroy(entries,true);
	return EXIT_SUCCESS;
}

static s32 compareEntries(const void *a,const void *b) {
	sLSFile *ea = *(sLSFile**)a;
	sLSFile *eb = *(sLSFile**)b;
	if(MODE_IS_DIR(ea->mode) == MODE_IS_DIR(eb->mode))
		return strcmp(ea->name,eb->name);
	if(MODE_IS_DIR(ea->mode))
		return -1;
	return 1;
}

static sVector *getEntries(const char *path) {
	sFile *p;
	sVector *entries = vec_create(sizeof(sLSFile*));
	p = file_get(path);
	if(p->isDir(p)) {
		sDirEntry *de;
		sVector *files = p->listFiles(p,flags & F_ALL);
		vforeach(files,de) {
			sFile *f = file_getIn(path,de->name);
			sLSFile *lsf = buildFile(f);
			vec_add(entries,&lsf);
			f->destroy(f);
		}
		vec_destroy(files,true);
	}
	else {
		sLSFile *lsf = buildFile(p);
		vec_add(entries,&lsf);
	}
	p->destroy(p);
	return entries;
}

static sLSFile *buildFile(sFile *f) {
	sFileInfo *i = f->getInfo(f);
	sLSFile *lf = heap_alloc(sizeof(sLSFile));
	lf->mode = i->mode;
	lf->modifytime = i->modifytime;
	lf->linkCount = i->linkCount;
	lf->inodeNo = i->inodeNo;
	lf->uid = i->uid;
	lf->gid = i->gid;
	if(MODE_IS_DIR(i->mode) && (flags & (F_DIRSIZE | F_DIRNUM))) {
		char path[MAX_PATH_LEN];
		f->getAbsolute(f,path,sizeof(path));
		sFile *d = file_get(path);
		if(flags & (F_DIRNUM)) {
			sVector *files = d->listFiles(d,flags & F_ALL);
			lf->size = files->count;
			vec_destroy(files,true);
		}
		else
			lf->size = getDirSize(d);
		d->destroy(d);
	}
	else
		lf->size = i->size;
	strcpy(lf->name,f->name(f));
	return lf;
}

static s32 getDirSize(sFile *d) {
	char path[MAX_PATH_LEN];
	s32 res = 0;
	sVector *files = d->listFiles(d,flags & F_ALL);
	if(flags & (F_DIRNUM))
		res = files->count;
	else {
		sDirEntry *e;
		d->getAbsolute(d,path,sizeof(path));
		vforeach(files,e) {
			sFile *f = file_getIn(path,e->name);
			if(f->isDir(f))
				res += getDirSize(f);
			else
				res += f->getInfo(f)->size;
			f->destroy(f);
		}
	}
	vec_destroy(files,true);
	return res;
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
	cout->writec(cout,' ');
}

static void printPerm(u16 mode,u16 fl,char c) {
	if((mode & fl) != 0)
		cout->writec(cout,c);
	else
		cout->writec(cout,'-');
}
