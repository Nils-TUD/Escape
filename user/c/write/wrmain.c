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
#include <esc/cmdargs.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <stdio.h>
#include <esc/proc.h>
#include <string.h>

#define BUF_SIZE 512

static char buffer[BUF_SIZE];

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-a] <file>\n",name);
	fprintf(stderr,"	-a: append to file\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	char path[MAX_PATH_LEN];
	char *filename = NULL;
	bool append = false;
	tFD fd;
	s32 i,c;

	if(argc < 2 || isHelpCmd(argc,argv))
		usage(argv[0]);
	for(i = 1; i < argc; i++) {
		if(strcmp(argv[i],"-a") == 0)
			append = true;
		else {
			if(filename != NULL)
				usage(argv[0]);
			filename = argv[i];
		}
	}
	if(filename == NULL)
		usage(argv[0]);

	abspath(path,MAX_PATH_LEN,filename);
	fd = open(path,IO_WRITE | (append ? IO_APPEND : (IO_TRUNCATE | IO_CREATE)));
	if(fd < 0)
		error("Unable to open '%s'",path);

	while((c = read(STDIN_FILENO,buffer,BUF_SIZE)) > 0) {
		if(write(fd,buffer,c) != c)
			error("Unable to write %d bytes",c);
	}

	close(fd);

	return EXIT_SUCCESS;
}
