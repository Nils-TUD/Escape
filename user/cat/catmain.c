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
#include <stdlib.h>

#define BUF_SIZE 512

int main(int argc,char *argv[]) {
	tFile *file;
	s32 count;
	char *path;
	char buffer[BUF_SIZE];

	if(argc != 1 && argc != 2) {
		fprintf(stderr,"Usage: %s [<file>]\n",argv[0]);
		return EXIT_FAILURE;
	}

	file = stdin;
	if(argc == 2) {
		path = abspath(argv[1]);
		file = fopen(path,"r");
		if(file == NULL) {
			printe("Unable to open '%s'",path);
			return EXIT_FAILURE;
		}
	}

	while((count = fread(buffer,sizeof(char),BUF_SIZE - 1,file)) > 0) {
		*(buffer + count) = '\0';
		printf("%s",buffer);
	}

	if(argc == 2)
		fclose(file);

	return EXIT_SUCCESS;
}
