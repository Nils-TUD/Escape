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
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <esc/cmdargs.h>
#include <stdlib.h>

#define BUF_SIZE 512

static void printFile(tFile *file);

int main(int argc,char *argv[]) {
	tFile *file;
	char *path;

	if(isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<file> ...]\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(argc < 2)
		printFile(stdin);
	else {
		s32 i;
		sFileInfo info;
		path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(path == NULL)
			error("Unable to allocate mem for path");
		for(i = 1; i < argc; i++) {
			abspath(path,MAX_PATH_LEN + 1,argv[i]);

			/* check if it's a directory */
			if(stat(path,&info) < 0) {
				printe("Unable to get info about '%s'",path);
				continue;
			}
			if(MODE_IS_DIR(info.mode)) {
				printe("'%s' is a directory!",path);
				continue;
			}

			file = fopen(path,"r");
			if(file == NULL) {
				printe("Unable to open '%s'",path);
				continue;
			}

			printFile(file);
			fclose(file);
		}
		free(path);
	}

	return EXIT_SUCCESS;
}

static void printFile(tFile *file) {
	s32 count;
	char buffer[BUF_SIZE];
	while((count = fread(buffer,sizeof(char),BUF_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}
}
