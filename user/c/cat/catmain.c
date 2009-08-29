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

int main(int argc,char *argv[]) {
	tFile *file;
	s32 count;
	char *path;
	char buffer[BUF_SIZE];

	if((argc != 1 && argc != 2) || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<file>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	file = stdin;
	if(argc == 2) {
		sFileInfo info;
		path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(path == NULL) {
			printe("Unable to allocate mem for path");
			return EXIT_FAILURE;
		}

		abspath(path,MAX_PATH_LEN + 1,argv[1]);

		/* check if it's a directory */
		if(getFileInfo(path,&info) < 0) {
			printe("Unable to get info about '%s'",path);
			return EXIT_FAILURE;
		}
		if(MODE_IS_DIR(info.mode)) {
			fprintf(stderr,"'%s' is a directory!\n",path);
			return EXIT_FAILURE;
		}

		file = fopen(path,"r");
		if(file == NULL) {
			printe("Unable to open '%s'",path);
			return EXIT_FAILURE;
		}

		free(path);
	}

	while((count = fread(buffer,sizeof(char),BUF_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}

	if(argc == 2)
		fclose(file);

	return EXIT_SUCCESS;
}
