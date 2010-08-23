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
#include <esc/fsinterface.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/date.h>
#include <esc/cmdargs.h>
#include <stdio.h>

#define MAX_DATE_LEN	64

static void stat_printDate(const char *title,u32 timestamp);
static const char *stat_getType(sFileInfo *info);

int main(int argc,char *argv[]) {
	sFileInfo info;
	char apath[MAX_PATH_LEN];

	if(argc != 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s <file>\n",argv[0]);
		return EXIT_FAILURE;
	}

	abspath(apath,MAX_PATH_LEN,argv[1]);
	if(stat(apath,&info) < 0)
		error("Unable to read file-information for '%s'",apath);

	printf("'%s' points to:\n",apath);
	printf("%-15s%d\n","Inode:",info.inodeNo);
	printf("%-15s%d\n","Device:",info.device);
	printf("%-15s%s\n","Type:",stat_getType(&info));
	printf("%-15s%d Bytes\n","Size:",info.size);
	printf("%-15s%d\n","Blocks:",info.blockCount);
	stat_printDate("Accessed:",info.accesstime);
	stat_printDate("Modified:",info.modifytime);
	stat_printDate("Created:",info.createtime);
	printf("%-15s%#o\n","Mode:",info.mode);
	printf("%-15s%d\n","GroupID:",info.gid);
	printf("%-15s%d\n","UserID:",info.uid);
	printf("%-15s%d\n","Hardlinks:",info.linkCount);
	printf("%-15s%d\n","BlockSize:",info.blockSize);

	return EXIT_SUCCESS;
}

static void stat_printDate(const char *title,u32 timestamp) {
	static char dateStr[MAX_DATE_LEN];
	sDate date = date_getOfTS(timestamp);
	date.format(&date,dateStr,MAX_DATE_LEN,"%a, %m/%d/%Y %H:%M:%S");
	printf("%-15s%s\n",title,dateStr);
}

static const char *stat_getType(sFileInfo *info) {
	if(MODE_IS_DIR(info->mode))
		return "Directory";
	if(MODE_IS_BLOCKDEV(info->mode))
		return "Block-Device";
	if(MODE_IS_CHARDEV(info->mode))
		return "Character-Device";
	if(MODE_IS_FIFO(info->mode))
		return "FIFO";
	if(MODE_IS_LINK(info->mode))
		return "Link";
	if(MODE_IS_SOCKET(info->mode))
		return "Socket";
	return "Regular File";
}
