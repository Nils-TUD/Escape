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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_DATE_LEN	64

static void printDate(const char *title,time_t timestamp);
static const char *getType(struct stat *info);
static void printFileInfo(const char *path,bool useOpen);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-o] <file>...\n",name);
	fprintf(stderr,"    -o: Use open and fstat instead of stat. On devices this has the\n");
	fprintf(stderr,"        effect that a channel is created and you get information about\n");
	fprintf(stderr,"        that channel instead of the device.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	bool useOpen = false;

	int opt;
	while((opt = getopt(argc,argv,"o")) != -1) {
		switch(opt) {
			case 'o': useOpen = true; break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	for(int i = optind; i < argc; ++i)
		printFileInfo(argv[i],useOpen);
	return EXIT_SUCCESS;
}

static void printFileInfo(const char *path,bool useOpen) {
	struct stat info;
	char apath[MAX_PATH_LEN];
	if(cleanpath(apath,MAX_PATH_LEN,path) < 0)
		error("cleanpath for '%s' failed",path);

	if(useOpen) {
		int fd = open(apath,O_RDONLY | O_NOFOLLOW);
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
	else if(lstat(apath,&info) < 0) {
		printe("stat of '%s' failed",apath);
		return;
	}

	printf("'%s' points to:\n",apath);
	printf("%-15s%d\n","Inode:",info.st_ino);
	printf("%-15s%d\n","Device:",info.st_dev);
	printf("%-15s%s\n","Type:",getType(&info));
	printf("%-15s%ld Bytes\n","Size:",info.st_size);
	printf("%-15s%d\n","Blocks:",info.st_blocks);
	printDate("Accessed:",info.st_atime);
	printDate("Modified:",info.st_mtime);
	printDate("Created:",info.st_ctime);
	printf("%-15s%#ho\n","Mode:",info.st_mode);
	printf("%-15s%hd\n","GroupID:",info.st_gid);
	printf("%-15s%hd\n","UserID:",info.st_uid);
	printf("%-15s%hd\n","Hardlinks:",info.st_nlink);
	printf("%-15s%hd\n","BlockSize:",info.st_blksize);
}

static void printDate(const char *title,time_t timestamp) {
	static char dateStr[MAX_DATE_LEN];
	struct tm *date = gmtime(&timestamp);
	strftime(dateStr,sizeof(dateStr),"%a, %m/%d/%Y %H:%M:%S",date);
	printf("%-15s%s\n",title,dateStr);
}

static const char *getType(struct stat *info) {
	if(S_ISDIR(info->st_mode))
		return "Directory";
	if(S_ISLNK(info->st_mode))
		return "Symbolic Link";
	if(S_ISBLK(info->st_mode))
		return "Block Device";
	if(S_ISCHR(info->st_mode))
		return "Character Device";
	if(S_ISFS(info->st_mode))
		return "Filesystem Device";
	if(S_ISSERV(info->st_mode))
		return "Service Device";
	if(S_ISMS(info->st_mode))
		return "Mountspace";
	if(S_ISIRQ(info->st_mode))
		return "Interrupt";
	return "Regular File";
}
