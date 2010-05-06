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
#include <esc/date.h>
#include <esc/cmdargs.h>
#include <esc/heap.h>
#include <esc/proc.h>
#include <esc/algo.h>
#include <width.h>
#include <messages.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <esc/exceptions/cmdargs.h>
#include <esc/exceptions/io.h>
#include <esc/io/file.h>
#include <esc/io/console.h>
#include <esc/util/cmdargs.h>
#include <esc/util/vector.h>

#define DATE_LEN			(SSTRLEN("2009-09-09 14:12") + 1)

/* for calculating the widths of the fields */
#define WIDTHS_COUNT		6
#define W_INODE				0
#define W_LINKCOUNT			1
#define W_UID				2
#define W_GID				3
#define W_SIZE				4
#define W_NAME				5

static s32 compareEntries(const void *a,const void *b);
static sVector *getEntries(const char *path,bool fall);
static void printMode(u16 mode);
static void printPerm(u16 mode,u16 flags,char c);

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-lia] [<path>]\n",name);
	cerr->writef(cerr,"	-l: long listing\n");
	cerr->writef(cerr,"	-i: print inode-numbers\n");
	cerr->writef(cerr,"	-a: print also '.' and '..'\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	char path[MAX_PATH_LEN];
	u32 widths[WIDTHS_COUNT] = {0};
	u32 pos,x;
	sVector *entries;
	sFile *f;
	sVTSize consSize;
	sCmdArgs *args;
	const char *filename;
	bool flong = 0,finode = 0,fall = 0;

	/* parse params */
	TRY {
		args = cmdargs_create(argc,argv,CA_MAX1_FREE);
		args->parse(args,"l i a",&flong,&finode,&fall);
		if(args->isHelp)
			usage(argv[0]);
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
		entries = getEntries(path,fall);
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
			sFileInfo *info = f->getInfo(f);
			if(!flong) {
				if((x = strlen(f->name(f))) > widths[W_NAME])
					widths[W_NAME] = x;
			}
			if(finode) {
				if((x = getnwidth(info->inodeNo)) > widths[W_INODE])
					widths[W_INODE] = x;
			}
			if(flong) {
				if((x = getuwidth(info->linkCount,10)) > widths[W_LINKCOUNT])
					widths[W_LINKCOUNT] = x;
				if((x = getuwidth(info->uid,10)) > widths[W_UID])
					widths[W_UID] = x;
				if((x = getuwidth(info->gid,10)) > widths[W_GID])
					widths[W_GID] = x;
				if((x = getnwidth(info->size)) > widths[W_SIZE])
					widths[W_SIZE] = x;
			}
		}
	}

	/* display */
	{
		vforeach(entries,f) {
			sFileInfo *info = f->getInfo(f);
			if(flong) {
				if(finode)
					cout->writef(cout,"%*d ",widths[W_INODE],info->inodeNo);
				printMode(info->mode);
				cout->writef(cout,"%*u ",widths[W_LINKCOUNT],info->linkCount);
				cout->writef(cout,"%*u ",widths[W_UID],info->uid);
				cout->writef(cout,"%*u ",widths[W_GID],info->gid);
				cout->writef(cout,"%*d ",widths[W_SIZE],info->size);
				{
					char dateStr[DATE_LEN];
					sDate date = date_getOfTS(info->modifytime);
					date.format(&date,dateStr,DATE_LEN,"%Y-%m-%d %H:%M");
					cout->writef(cout,"%s ",dateStr);
				}
				if(MODE_IS_DIR(info->mode))
					cout->writef(cout,"\033[co;9]%s\033[co]",f->name(f));
				else if(info->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
					cout->writef(cout,"\033[co;2]%s\033[co]",f->name(f));
				else
					cout->writes(cout,f->name(f));
				cout->writec(cout,'\n');
			}
			else {
				/* if the entry does not fit on the line, use next */
				if(pos + widths[W_NAME] + widths[W_INODE] + 2 >= consSize.width) {
					cout->writec(cout,'\n');
					pos = 0;
				}
				if(finode)
					cout->writef(cout,"%*d ",widths[W_INODE],info->inodeNo);
				if(MODE_IS_DIR(info->mode))
					cout->writef(cout,"\033[co;9]%-*s\033[co]",widths[W_NAME] + 1,f->name(f));
				else if(info->mode & (MODE_OWNER_EXEC | MODE_GROUP_EXEC | MODE_OTHER_EXEC))
					cout->writef(cout,"\033[co;2]%-*s\033[co]",widths[W_NAME] + 1,f->name(f));
				else
					cout->writef(cout,"%-*s",widths[W_NAME] + 1,f->name(f));
				pos += widths[W_NAME] + widths[W_INODE] + 2;
			}
		}
	}

	if(!flong)
		cout->writec(cout,'\n');

	vec_destroy(entries,true);
	return EXIT_SUCCESS;
}

static s32 compareEntries(const void *a,const void *b) {
	sFile *ea = *(sFile**)a;
	sFile *eb = *(sFile**)b;
	if(ea->isDir(ea) == eb->isDir(eb))
		return strcmp(ea->name(ea),eb->name(eb));
	if(ea->isDir(ea))
		return -1;
	return 1;
}

static sVector *getEntries(const char *path,bool fall) {
	sFile *p;
	sVector *entries = vec_create(sizeof(sFile*));
	p = file_get(path);
	if(p->isDir(p)) {
		sDirEntry *de;
		sVector *files = p->listFiles(p,fall);
		vforeach(files,de) {
			sFile *f = file_getIn(path,de->name);
			vec_add(entries,&f);
		}
		vec_destroy(files,true);
	}
	else
		vec_add(entries,&p);
	return entries;
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

static void printPerm(u16 mode,u16 flags,char c) {
	if((mode & flags) != 0)
		cout->writec(cout,c);
	else
		cout->writec(cout,'-');
}
