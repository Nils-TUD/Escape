/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <esc/fsinterface.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_DATE_LEN	64

static void printDate(const char *title,time_t timestamp);
static const char *getType(sFileInfo *info);
static void printFileInfo(const char *path,bool useOpen);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-o] <file>...\n",name);
	fprintf(stderr,"    -o: Use open and fstat instead of stat. On devices this has the\n");
	fprintf(stderr,"        effect that a channel is created and you get information about\n");
	fprintf(stderr,"        that channel instead of the device.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	bool useOpen = false;

	int res = ca_parse(argc,argv,0,"o",&useOpen);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	size_t count = count = ca_getFreeCount();
	if(count == 0 || ca_hasHelp())
		usage(argv[0]);

	for(size_t i = 0; i < count; ++i)
		printFileInfo(ca_getFree()[i],useOpen);
	return EXIT_SUCCESS;
}

static void printFileInfo(const char *path,bool useOpen) {
	sFileInfo info;
	char apath[MAX_PATH_LEN];
	cleanpath(apath,MAX_PATH_LEN,path);

	if(useOpen) {
		int fd = open(apath,O_RDONLY);
		if(fd < 0) {
			printe("open of '%s' for reading failed",apath);
			return;
		}
		if(fstat(fd,&info) < 0) {
			printe("fstat of '%s' failed",apath);
			return;
		}
		close(fd);
	}
	else if(stat(apath,&info) < 0) {
		printe("stat of '%s' failed",apath);
		return;
	}

	printf("'%s' points to:\n",apath);
	printf("%-15s%d\n","Inode:",info.inodeNo);
	printf("%-15s%d\n","Device:",info.device);
	printf("%-15s%s\n","Type:",getType(&info));
	printf("%-15s%d Bytes\n","Size:",info.size);
	printf("%-15s%hd\n","Blocks:",info.blockCount);
	printDate("Accessed:",info.accesstime);
	printDate("Modified:",info.modifytime);
	printDate("Created:",info.createtime);
	printf("%-15s%#ho\n","Mode:",info.mode);
	printf("%-15s%hd\n","GroupID:",info.gid);
	printf("%-15s%hd\n","UserID:",info.uid);
	printf("%-15s%hd\n","Hardlinks:",info.linkCount);
	printf("%-15s%hd\n","BlockSize:",info.blockSize);
}

static void printDate(const char *title,time_t timestamp) {
	static char dateStr[MAX_DATE_LEN];
	struct tm *date = gmtime(&timestamp);
	strftime(dateStr,sizeof(dateStr),"%a, %m/%d/%Y %H:%M:%S",date);
	printf("%-15s%s\n",title,dateStr);
}

static const char *getType(sFileInfo *info) {
	if(S_ISDIR(info->mode))
		return "Directory";
	if(S_ISBLK(info->mode))
		return "Block-Device";
	if(S_ISCHR(info->mode))
		return "Character-Device";
	if(S_ISFS(info->mode))
		return "Filesystem-Device";
	if(S_ISSERV(info->mode))
		return "Service-Device";
	return "Regular File";
}
