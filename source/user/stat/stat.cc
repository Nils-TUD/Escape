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

#include <esc/file.h>
#include <esc/stream/std.h>
#include <usergroup/usergroup.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdlib.h>
#include <time.h>

#define MAX_DATE_LEN	64

using namespace esc;

static void printFileInfo(const char *path,uint flags);

static void usage(const char *name) {
	serr << "Usage: " << name << " [-cL] <file>...\n";
	serr << "    -c: do not use O_NOCHAN to get the file info. On devices this has the\n";
	serr << "        effect that a channel is created and you get information about\n";
	serr << "        that channel instead of the device.\n";
	serr << "    -L: dereference symbolic links.\n";
	exit(EXIT_FAILURE);
}

static sNamedItem *userList;
static sNamedItem *groupList;

int main(int argc,char *argv[]) {
	uint flags = O_NOCHAN | O_NOFOLLOW;

	int opt;
	while((opt = getopt(argc,argv,"cL")) != -1) {
		switch(opt) {
			case 'c':
				flags &= ~O_NOCHAN;
				flags |= O_RDONLY;
				break;
			case 'L':
				flags &= ~O_NOFOLLOW;
				break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	userList = usergroup_parse(USERS_PATH,nullptr);
	if(!userList)
		errmsg("Warning: unable to parse users from file");
	groupList = usergroup_parse(GROUPS_PATH,nullptr);
	if(!groupList)
		errmsg("Unable to parse groups from file");

	for(int i = optind; i < argc; ++i)
		printFileInfo(argv[i],flags);
	return EXIT_SUCCESS;
}

static const char *getType(mode_t mode) {
	if(S_ISDIR(mode))
		return "directory";
	if(S_ISLNK(mode))
		return "symbolic link";
	if(S_ISBLK(mode))
		return "block device";
	if(S_ISCHR(mode))
		return "character device";
	if(S_ISFS(mode))
		return "filesystem device";
	if(S_ISSERV(mode))
		return "service device";
	if(S_ISIRQ(mode))
		return "interrupt";
	return "regular file";
}

static void printDate(const char *title,time_t timestamp) {
	static char dateStr[MAX_DATE_LEN];
	struct tm *date = gmtime(&timestamp);
	strftime(dateStr,sizeof(dateStr),"%Y-%m-%d %H:%M:%S",date);
	sout << title << ": " << dateStr << "\n";
}

static void printFileInfo(const char *path,uint flags) {
	struct stat i;

	int fd = open(path,flags);
	if(fd < 0) {
		printe("Unable to open '%s'",path);
		return;
	}
	int res = fstat(fd,&i);
	close(fd);
	if(res < 0) {
		printe("fstat for '%s' failed",path);
		return;
	}

	sout << "  File: " << path;
	if(S_ISLNK(i.st_mode)) {
		char lnk[MAX_PATH_LEN];
		if(readlink(path,lnk,sizeof(lnk)) < 0)
			sout << " -> ??";
		else
			sout << " -> " << lnk;
	}
	sout << "\n";

	sout << "  Size: " << fmt(i.st_size,"-",16)
		 << "Blocks: " << fmt(i.st_blocks,"-",11)
		 << "Block: "  << fmt(i.st_blksize,"-",7)
		 << getType(i.st_mode) << "\n";

	sout << "Device: " << fmt(i.st_dev,"-",16)
		 << "Inode: " << fmt(i.st_ino,"-",12)
		 << "Links: " << i.st_nlink << "\n";

	sNamedItem *u = userList ? usergroup_getById(userList,i.st_uid) : nullptr;
	sNamedItem *g = userList ? usergroup_getById(groupList,i.st_gid) : nullptr;

	sout << "Access: (" << fmt(i.st_mode & 0777,"0o",4) << "/";
	file::printMode(sout,i.st_mode);
	sout << ")  Uid: (" << fmt(i.st_uid,5) << "/" << fmt(u ? u->name : "??",8) << ")"
		 << "   Gid: (" << fmt(i.st_gid,5) << "/" << fmt(g ? g->name : "??",8) << ")\n";

	printDate("Access",i.st_atime);
	printDate("Modify",i.st_mtime);
	printDate("Create",i.st_ctime);
}
